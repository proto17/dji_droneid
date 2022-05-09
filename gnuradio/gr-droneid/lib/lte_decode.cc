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
#include <droneid/lte_decode.h>
#include <iostream>

namespace gr {
    namespace droneid {

        lte_decode::lte_decode() {
            d1_.resize(TURBO_DECODER_BIT_COUNT);
            d2_.resize(TURBO_DECODER_BIT_COUNT);
            d3_.resize(TURBO_DECODER_BIT_COUNT);

            turbo_decoder_input_.resize(TURBO_DECODER_INPUT_BIT_COUNT);
            decoded_bytes_.resize(EXPECTED_PAYLOAD_BYTES);

            rate_matcher_ = lte_rate_matcher_alloc();
            turbo_decoder_ = alloc_tdec();

            rate_matcher_io_ = {
                    .D = TURBO_DECODER_BIT_COUNT,
                    .E = TURBO_DECODER_INPUT_BIT_COUNT,
                    .d = {&d1_[0], &d2_[0], &d3_[0]},
                    .e = &turbo_decoder_input_[0]
            };
        }

        lte_decode::~lte_decode() {
            if (rate_matcher_ != nullptr) {
                lte_rate_matcher_free(rate_matcher_);
            }

            if (turbo_decoder_ != nullptr) {
                free_tdec(turbo_decoder_);
            }
        }

        std::vector<uint8_t> lte_decode::decode(const std::vector<int8_t> &bits) {
            if (bits.size() != TURBO_DECODER_INPUT_BIT_COUNT) {
                std::ostringstream err;
                err << "Turbo decoder expected " << TURBO_DECODER_INPUT_BIT_COUNT << " but got " << bits.size();
                throw std::runtime_error(err.str());
            }

            int8_t bit_lut[2] = {-63, 63};
            std::vector<int8_t> bits_copy = bits;
            std::for_each(bits_copy.begin(), bits_copy.end(), [&](int8_t & bit){
                bit = bit_lut[bit];
            });

            for (auto & i : bits_copy) {
                std::cout << (short)i << " ";
            }
            std::cout << "\n";

            std::copy(bits_copy.begin(), bits_copy.end(), turbo_decoder_input_.begin());
            lte_rate_match_fw(rate_matcher_, &rate_matcher_io_, 0);
            const int decode_status = lte_turbo_decode(turbo_decoder_, EXPECTED_PAYLOAD_BITS, TURBO_ITERATIONS,
                             &decoded_bytes_[0], &d1_[0], &d2_[0], &d3_[0]);

            fprintf(stdout, "MOO: ");
            for (const auto & i : decoded_bytes_) {
                fprintf(stdout, "%02x", i);
            }
            fprintf(stdout, "\n");

            if (decode_status != 0) {
                throw std::runtime_error("Failed to remove Turbo code.  Status: " + std::to_string(decode_status));
            }

            const uint32_t calculated_crc = CRC::Calculate(&decoded_bytes_[0], decoded_bytes_.size(), CRC::CRC_24_LTEA());
            if (calculated_crc != 0) {
                return {};
            }

            return decoded_bytes_;
        }

    } /* namespace droneid */
} /* namespace gr */

