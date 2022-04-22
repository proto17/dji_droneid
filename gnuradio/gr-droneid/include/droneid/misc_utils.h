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

#ifndef INCLUDED_DRONEID_MISC_UTILS_H
#define INCLUDED_DRONEID_MISC_UTILS_H

#include <droneid/api.h>
#include <vector>
#include <complex>

namespace gr {
    namespace droneid {

        /*!
         * \brief <+description+>
         *
         */
        class DRONEID_API misc_utils {
        public:
            static constexpr double CARRIER_SPACING = 15e3;
            static constexpr uint32_t OCCUPIED_CARRIERS_EXC_DC = 600;
            static constexpr uint32_t OCCUPIED_CARRIERS_INC_DC = 601;

            static uint32_t get_long_cp_len(double sample_rate);
            static uint32_t get_short_cp_len(double sample_rate);
            static uint32_t get_fft_size(double sample_rate);

            static std::vector<std::complex<float>> create_zc_sequence(double sample_rate, uint32_t root);
            static std::vector<std::complex<float>> conj(const std::vector<std::complex<float>> & input);

            static void write_samples(const std::string &path, const std::complex<float> * samples, uint32_t element_count);
            static void write_samples(const std::string & path, const std::vector<std::complex<float>> & samples);


            misc_utils();

            ~misc_utils();

        private:
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_MISC_UTILS_H */

