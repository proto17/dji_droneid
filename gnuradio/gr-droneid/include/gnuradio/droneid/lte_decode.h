/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_LTE_DECODE_H
#define INCLUDED_DRONEID_LTE_DECODE_H

#include <gnuradio/droneid/api.h>
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
class DRONEID_API lte_decode
{
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
