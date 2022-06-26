/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "normalized_xcorr_estimate_impl.h"
#include <volk/volk.h>
#include <numeric>
#include <droneid/misc_utils.h>

namespace gr {
    namespace droneid {

        normalized_xcorr_estimate::sptr
        normalized_xcorr_estimate::make(const std::vector<gr_complex> &taps) {
            return gnuradio::get_initial_sptr
                    (new normalized_xcorr_estimate_impl(taps));
        }


        /*
         * The private constructor
         */
        normalized_xcorr_estimate_impl::normalized_xcorr_estimate_impl(const std::vector<gr_complex> &taps)
                : gr::block("dot_prod",
                            gr::io_signature::make(1, 1, sizeof(gr_complex)),
                            gr::io_signature::make(1, 1, sizeof(gr_complex))),
                  taps_(taps), window_size_(taps.size()) {

            // Remove the mean from the taps, conjugate the taps, and calculate the variance ahead of time
            const auto mean =
                    std::accumulate(taps_.begin(), taps_.end(), gr_complex{0, 0}) / static_cast<float>(taps_.size());

            for (auto & tap : taps_) {
                tap = std::conj(tap) - mean;
            }

            taps_var_ = misc_utils::var_no_mean(taps_);

            // Create some constants to enable the use of multiplies instead of divides later
            window_size_recip_ = 1.0f / static_cast<float>(window_size_);
            window_size_recip_complex_ = gr_complex{window_size_recip_, 0};
        }

        /*
         * Our virtual destructor.
         */
        normalized_xcorr_estimate_impl::~normalized_xcorr_estimate_impl() {
        }

        int
        normalized_xcorr_estimate_impl::general_work(int noutput_items,
                                                gr_vector_int &ninput_items,
                                                gr_vector_const_void_star &input_items,
                                                gr_vector_void_star &output_items) {
            const auto *in = (const gr_complex *) input_items[0];
            auto *out = (gr_complex *) output_items[0];

            consume_each(noutput_items);

            // This is how the remaining samples are buffered between calls.  It's important to realize that this algo
            // needs <window_size> samples to be able to produce one output value.  This means that there will always
            // be unused samples at the end of each function call that need to be held onto until the next call.  The
            // hope was that set_history() took care of this, but it does not.  So, the remaining samples from the last
            // call are stored in <buffer_>.  The <in> buffer can't hold more samples (it's not known how many samples
            // wide the buffer is) so in order to use the old samples without jumping through very slow hoops, the new
            // samples are appended to the old samples.
            buffer_.insert(buffer_.end(), in, in + noutput_items);

            // Exit early if there aren't enough samples to process.
            if (buffer_.size() < window_size_) {
                return 0;
            }

            // Figure out how many windows worth of data can be processed.  It's possible that this specific call
            // doesn't have enough storage in its output buffer to hold all the samples that could be processed.  For
            // this reason the min of the available output buffer space and number of windows that could be processed
            // must be used.
            const auto num_steps = std::min(static_cast<uint64_t>(noutput_items), buffer_.size() - window_size_);

            // Resize the buffers as needed
            if (sums_.size() < num_steps) {
                sums_.resize(num_steps);
                abs_squared_.resize(num_steps + window_size_);
                vars_.resize(num_steps);
            }

            // TODO(24June2022): There are <window_size-1> extra operations happening on each call.  This comes from the
            //                   fact that some of these computations are being done on samples that are going to be
            //                   used again on the next function call.  Would be a good idea to buffer the abs squared
            //                   and maybe the running variance average.

            // What is happening below is roughly the following:
            //
            // for idx = 1:length(buffer_) - window_size_
            //   window = buffer_(idx:idx + window_size_ - 1);
            //   variance = sum(abs(window).^2) / window_size_;
            //   dot_prod = sum(window .* taps_) / window_size_;
            //   out(idx) = dot_prod / sqrt(variance * taps_var_);
            // end
            //
            // But the variance is calculated as a running sum.  The first variance has to be calculated the hard way,
            // and then every iteration of the loop will subtract off the left-most element of the window that just
            // dropped off, and adds on the new right-most element in the window.
            //
            // Doing this calculation of the first element outside the loop prevents needing a conditional in the
            // critical section

            // Calculate the first variance the hard way
            volk_32fc_magnitude_squared_32f(&abs_squared_[0], &buffer_[0], num_steps + window_size_);
            auto running_var = std::accumulate(abs_squared_.begin(), abs_squared_.begin() + window_size_, 0.f);
            vars_[0] = running_var;

            // Calculate the first dot product
            volk_32fc_x2_dot_prod_32fc(&out[0], &buffer_[0], &taps_[0], window_size_);

            // Calculate the running abs value sum and dot product for the remaining samples
            for (uint32_t idx = 1; idx < num_steps; idx++) {
                // sum(abs(window).^2)
                running_var = running_var - abs_squared_[idx - 1] + abs_squared_[idx + window_size_];
                vars_[idx] = running_var;

                // Compute tue dot product of the current window and the filter taps
                // sum(window .* taps_)
                volk_32fc_x2_dot_prod_32fc(&out[idx], &buffer_[idx], &taps_[0], window_size_);
            }

            // Scale the dot product down
            volk_32fc_s32fc_multiply_32fc(&out[0], &out[0], window_size_recip_complex_, num_steps);

            // Scale the variance sums down
            volk_32f_s32f_multiply_32f(&vars_[0], &vars_[0], window_size_recip_, num_steps);

            // Multiply each variance by the tap variances then take the reciprocal
            volk_32f_s32f_multiply_32f(&vars_[0], &vars_[0], taps_var_, num_steps);

            // Take the square root of the product of the two variances
            volk_32f_sqrt_32f(&vars_[0], &vars_[0], num_steps);

            // There's no VOLK function for the reciprocal operation.  This is being done so that a multiply can be
            // used next to divide the dot product results by the sqrt calculated above
            for (auto & var : vars_) {
                var = 1.0f / var;
            }

            // Divide by the square root above
            volk_32fc_32f_multiply_32fc(&out[0], &out[0], &vars_[0], num_steps);

            // Remove all the samples that have been processed from the buffer.  Leaving just the last <window_size_-1>
            // samples for the next call
            buffer_.erase(buffer_.begin(), buffer_.begin() + num_steps);

            // Tell runtime system how many output items we produced.
            return num_steps;
        }

    } /* namespace droneid */
} /* namespace gr */

