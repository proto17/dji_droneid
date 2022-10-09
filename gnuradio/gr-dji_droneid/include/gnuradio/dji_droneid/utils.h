/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DJI_DRONEID_UTILS_H
#define INCLUDED_DJI_DRONEID_UTILS_H

#include <gnuradio/dji_droneid/api.h>
#include <complex>
#include <cstdint>
#include <vector>
#include <memory>

namespace gr {
namespace dji_droneid {

/*!
 * \brief <+description+>
 *
 */
class DJI_DRONEID_API utils
{
public:
    utils();
    ~utils();

    static constexpr float CARRIER_SPACING = 15e3;
    static constexpr uint32_t DATA_CARRIER_COUNT = 600;
    static constexpr std::complex<float> I = std::complex<float>(0, 1);

    static uint32_t get_fft_size(float sample_rate);
    static float get_sample_rate(uint32_t fft_size);
    static std::vector<uint32_t> get_data_carrier_indices(float sample_rate);
    static std::vector<std::complex<float>> zadoff_chu(uint32_t root, uint32_t length);
    static std::vector<std::complex<float>> create_zc(uint32_t fft_size, uint8_t symbol_idx);
    static std::pair<uint16_t, uint16_t> get_cyclic_prefix_lengths(float sample_rate);

private:
};

} // namespace dji_droneid
} // namespace gr



#endif /* INCLUDED_DJI_DRONEID_UTILS_H */
