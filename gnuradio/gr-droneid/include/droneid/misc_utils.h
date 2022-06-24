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
#include <array>
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

            static std::vector<std::complex<float>> create_gaussian_noise(uint32_t sample_count);

            static std::vector<std::complex<float>> create_zc_sequence(double sample_rate, uint32_t root);
            static std::vector<std::complex<float>> conj(const std::vector<std::complex<float>> & input);

            static std::complex<float> mean(const std::vector<std::complex<float>> & samples);
            static float var(const std::vector<std::complex<float>> & samples);
            static float var_no_mean(const std::vector<std::complex<float>> & samples);

            static std::complex<float> mean(const std::complex<float> * samples, uint32_t sample_count);
            static float var(const std::complex<float> * samples, uint32_t sample_count);

            static float var_no_mean(const std::complex<float> * samples, uint32_t sample_count);

            static void write(const std::string & path, const void * element, uint32_t element_size, uint32_t element_count);
            static void write(const std::string & path, const std::vector<uint32_t> & elements);
            static void write_samples(const std::string &path, const std::complex<float> * samples, uint32_t element_count);
            static void write_samples(const std::string & path, const std::vector<std::complex<float>> & samples);

            static std::vector<std::complex<float>> read_samples(const std::string & file_path, uint32_t offset, uint32_t total_samples);

            static std::vector<uint32_t> get_data_carrier_indices(uint32_t fft_size);
            static std::vector<std::complex<float>> extract_data_carriers(const std::vector<std::complex<float>> & symbol, uint32_t fft_size);

            static std::string bit_vec_to_string(const std::vector<int8_t> & bit_vec);

            static void print_bits(const std::vector<int8_t> & bits);

            static uint32_t find_zc_seq_start_idx(const std::vector<std::complex<float>> & samples, double sample_rate);
            static double radians_to_hz(double radians, double sample_rate);
            static double hz_to_radians(double frequency, double sample_rate);
            static std::vector<uint32_t> get_cyclic_prefix_schedule(double sample_rate);

            static std::vector<std::pair<std::vector<std::complex<float>>, std::vector<std::complex<float>>>>
                extract_ofdm_symbol_samples(const std::vector<std::complex<float>> & samples, double sample_rate, uint32_t offset);

            static std::vector<std::complex<float>> calculate_channel(const std::vector<std::complex<float>> & symbol, double sample_rate, uint32_t symbol_idx);
            static std::vector<int8_t> qpsk_to_bits(const std::vector<std::complex<float>> & samples);
            static std::vector<float> angle(const std::vector<std::complex<float>> & samples) {
                std::vector<float> angles(samples.size());

                for (uint32_t idx = 0; idx < samples.size(); idx++) {
                    angles[idx] = std::arg(samples[idx]);
                }

                return angles;
            }

            static std::vector<float> abs_squared(const std::vector<std::complex<float>> & samples);
            static std::vector<float> abs_squared(const std::vector<std::complex<float>> && samples);

            misc_utils();

            ~misc_utils();

        private:
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_MISC_UTILS_H */

