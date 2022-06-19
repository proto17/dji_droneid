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

#ifndef INCLUDED_DRONEID_NORMALIZED_XCORR_H
#define INCLUDED_DRONEID_NORMALIZED_XCORR_H

#include <droneid/api.h>
#include <vector>
#include <complex>

namespace gr {
    namespace droneid {

        /*!
         * \brief <+description+>
         *
         */
        class DRONEID_API normalized_xcorr {
        public:
            using sample_t = float;
            using complex_t = std::complex<sample_t>;
            using complex_vec_t = std::vector<complex_t>;

            explicit normalized_xcorr(const complex_vec_t & filter_taps);

            uint32_t run(const complex_t * samples, uint32_t sample_count, sample_t * output_samples);

            ~normalized_xcorr();
        protected:
            complex_vec_t taps_;
            double taps_var_;
            uint32_t window_size_;

            complex_vec_t temp_;
            complex_vec_t scores_;
        private:
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_NORMALIZED_XCORR_H */

