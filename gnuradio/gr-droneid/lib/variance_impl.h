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

#ifndef INCLUDED_DRONEID_VARIANCE_IMPL_H
#define INCLUDED_DRONEID_VARIANCE_IMPL_H

#include <droneid/variance.h>

namespace gr {
    namespace droneid {

        class variance_impl : public variance {
        private:
            const uint32_t window_size_;
            const float window_size_recip_;

            std::vector<gr_complex> buffer_;
            std::vector<float> abs_squared_;
            std::vector<float> sum_;

        public:
            variance_impl(uint32_t window_size);

            ~variance_impl();

            int general_work(int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) override;

        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_VARIANCE_IMPL_H */

