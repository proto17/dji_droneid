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

#ifndef INCLUDED_DRONEID_LTE_DECODE_H
#define INCLUDED_DRONEID_LTE_DECODE_H

#include <droneid/api.h>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include <turbofec/rate_match.h>
#include <turbofec/turbo.h>

#ifdef __cplusplus
}
#endif

// This line is required before the include of CRC.h so that it actually loads the CRC_24_LTEA definition
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
#include <CRC.h>

namespace gr {
    namespace droneid {

        /*!
         * \brief <+description+>
         *
         */
        class DRONEID_API lte_decode {
        protected:
            static constexpr uint32_t TURBO_ITERATIONS = 4;
            static constexpr uint32_t TURBO_DECODER_BIT_COUNT = 1412;
            static constexpr uint32_t TURBO_DECODER_INPUT_BIT_COUNT = 7200;
            static constexpr uint32_t EXPECTED_PAYLOAD_BYTES = 176;
            static constexpr uint32_t EXPECTED_PAYLOAD_BITS = EXPECTED_PAYLOAD_BYTES * 8;

            std::vector<int8_t> d1_, d2_, d3_;
            std::vector<int8_t> turbo_decoder_input_;
            std::vector<uint8_t> decoded_bytes_;

            struct lte_rate_matcher * rate_matcher_ = nullptr;
            struct tdecoder * turbo_decoder_ = nullptr;
            struct lte_rate_matcher_io rate_matcher_io_;
        public:
            lte_decode();
            ~lte_decode();

            std::vector<uint8_t> decode(const std::vector<int8_t> & bits);
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_LTE_DECODE_H */

