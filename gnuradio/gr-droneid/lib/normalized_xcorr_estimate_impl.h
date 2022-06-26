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

#ifndef INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_IMPL_H
#define INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_IMPL_H

#include <droneid/normalized_xcorr_estimate.h>

namespace gr {
    namespace droneid {

        class normalized_xcorr_estimate_impl : public normalized_xcorr_estimate {
        private:
            const uint32_t window_size_;
            float taps_var_;
            float window_size_recip_;
            gr_complex window_size_recip_complex_;
            std::vector<gr_complex> taps_;
            std::vector<gr_complex> sums_;
            std::vector<float> vars_;
            std::vector<float> abs_squared_;
            std::vector<gr_complex> buffer_;
            const gr_complex zero_complex_ = gr_complex{0, 0};
            // Nothing to declare in this block.

        public:
            normalized_xcorr_estimate_impl(const std::vector<gr_complex> & taps);

            ~normalized_xcorr_estimate_impl();

            int general_work(int noutput_items,
                             gr_vector_int &ninput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items);

        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_IMPL_H */

