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
#include "dot_prod_impl.h"
#include <volk/volk.h>
#include <numeric>
#include <droneid/misc_utils.h>

namespace gr {
    namespace droneid {

        dot_prod::sptr
        dot_prod::make(const std::vector<gr_complex> &taps) {
            return gnuradio::get_initial_sptr
                    (new dot_prod_impl(taps));
        }


        /*
         * The private constructor
         */
        dot_prod_impl::dot_prod_impl(const std::vector<gr_complex> &taps)
                : gr::block("dot_prod",
                            gr::io_signature::make(1, 1, sizeof(gr_complex)),
                            gr::io_signature::make(1, 1, sizeof(gr_complex))),
                  taps_(taps), window_size_(taps.size()) {
            const auto mean =
                    std::accumulate(taps_.begin(), taps_.end(), gr_complex{0, 0}) / static_cast<float>(taps_.size());

            for (auto & tap : taps_) {
                tap = std::conj(tap) - mean;
            }

            taps_var_ = misc_utils::var_no_mean(taps_);
            window_size_recip_ = 1.0f / static_cast<float>(window_size_);
            window_size_recip_complex_ = gr_complex{window_size_recip_, 0};
            temp_.resize(window_size_);
        }

        /*
         * Our virtual destructor.
         */
        dot_prod_impl::~dot_prod_impl() {
        }

        int
        dot_prod_impl::general_work(int noutput_items,
                                    gr_vector_int &ninput_items,
                                    gr_vector_const_void_star &input_items,
                                    gr_vector_void_star &output_items) {
            const auto *in = (const gr_complex *) input_items[0];
            auto *out = (gr_complex *) output_items[0];

            consume_each(noutput_items);

            buffer_.insert(buffer_.end(), in, in + noutput_items);

            if (buffer_.size() < window_size_) {
                return 0;
            }

            const auto num_steps = std::min(static_cast<uint64_t>(noutput_items), buffer_.size() - window_size_);

            if (sums_.size() < num_steps) {
                sums_.resize(num_steps);
                abs_squared_.resize(num_steps + window_size_);
                vars_.resize(num_steps);
            }

            // There are window_size - 1 extra calls each time!!!
            volk_32fc_magnitude_squared_32f(&abs_squared_[0], &buffer_[0], num_steps + window_size_);
            auto running_var = std::accumulate(abs_squared_.begin(), abs_squared_.begin() + window_size_, 0.f);
            vars_[0] = running_var;

            volk_32fc_x2_dot_prod_32fc(&out[0], &buffer_[0], &taps_[0], window_size_);
            out[0] /= static_cast<float>(window_size_);
            out[0] /= sqrtf((vars_[0] / static_cast<float>(window_size_)) * taps_var_);

            for (uint32_t idx = 1; idx < num_steps; idx++) {
                running_var = running_var - abs_squared_[idx - 1] + abs_squared_[idx + window_size_];
                vars_[idx] = running_var;

                const auto var = misc_utils::var_no_mean(&buffer_[idx], window_size_);

                // dot_prod = sum(left .* conj(right))
                volk_32fc_x2_dot_prod_32fc(&out[idx], &buffer_[idx], &taps_[0], window_size_);
                out[idx] = out[idx] / static_cast<float>(window_size_);
                out[idx] = out[idx] / sqrtf(var * taps_var_);
            }

//            // Scale the dot product down
//            // dot_prod = dot_prod / window_size;
//            volk_32fc_s32fc_multiply_32fc(&out[0], &out[0], window_size_recip_complex_, num_steps);
//
//            // Scale the variance sums down
//            // var = var / window_size
//            volk_32f_s32f_multiply_32f(&vars_[0], &vars_[0], window_size_recip_, num_steps);
//
//            // Multiply each variance by the tap variances then take the reciprocal
//            volk_32f_s32f_multiply_32f(&vars_[0], &vars_[0], taps_var_, num_steps);
//
//            // Take the square root of the product of the two variances
//            volk_32f_sqrt_32f(&vars_[0], &vars_[0], vars_.size());
//
//            volk_32f_s32f_multiply_32f(&vars_[0], &vars_[0], window_size_recip_, num_steps);


//            volk_32fc_32f_multiply_32fc(&out[0], &out[0], &vars_[0], num_steps);

            buffer_.erase(buffer_.begin(), buffer_.begin() + num_steps);

            // Tell runtime system how many output items we produced.
            return num_steps;
        }

    } /* namespace droneid */
} /* namespace gr */

