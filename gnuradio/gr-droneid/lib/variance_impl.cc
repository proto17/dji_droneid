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
#include "variance_impl.h"

#include <volk/volk.h>

namespace gr {
    namespace droneid {

        variance::sptr
        variance::make(uint32_t window_size) {
            return gnuradio::get_initial_sptr
                    (new variance_impl(window_size));
        }

        /*
         * The private constructor
         */
        variance_impl::variance_impl(uint32_t window_size)
                : gr::block("variance",
                                 gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                 gr::io_signature::make(1, 1, sizeof(float))),
                                 window_size_(window_size),
                                 window_size_recip_(1.0f / static_cast<float>(window_size)) {

        }

        /*
         * Our virtual destructor.
         */
        variance_impl::~variance_impl() {
        }

        int variance_impl::general_work(int noutput_items, gr_vector_int &ninput_items,
                                        gr_vector_const_void_star &input_items, gr_vector_void_star &output_items) {
            const auto * in = (gr_complex *) input_items[0];
            auto * out = (float *) output_items[0];

            buffer_.insert(buffer_.end(), in, in + noutput_items);

            if (buffer_.size() < window_size_) {
                consume_each(noutput_items);
                return 0;
            }

            const auto num_steps = buffer_.size() - window_size_;

            if (abs_squared_.size() < buffer_.size()) {
                abs_squared_.resize(buffer_.size());
                sum_.resize(num_steps);
            }

            volk_32fc_magnitude_squared_32f(&abs_squared_[0], &buffer_[0], buffer_.size());
            float running_sum;
            volk_32f_accumulator_s32f_a(&running_sum, &abs_squared_[0], window_size_);
            sum_[0] = running_sum;

            for (uint32_t idx = 1; idx < num_steps; idx++) {
                running_sum = running_sum - abs_squared_[idx - 1] + abs_squared_[idx + window_size_];
                sum_[idx] = running_sum;
            }

            volk_32f_s32f_multiply_32f(out, &sum_[0], window_size_recip_, num_steps);

            buffer_.erase(buffer_.begin(), buffer_.begin() + num_steps);

            consume_each(noutput_items);
            return num_steps;
        }

    } /* namespace droneid */
} /* namespace gr */

