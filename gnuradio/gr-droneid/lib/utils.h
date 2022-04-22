//
// Created by main on 4/20/22.
//

#ifndef GR_DRONEID_UTILS_H
#define GR_DRONEID_UTILS_H

#include <cstdint>
#include <vector>
#include <complex>

static constexpr double CARRIER_SPACING = 15e3;
static constexpr uint32_t OCCUPIED_CARRIERS_EXC_DC = 600;
static constexpr uint32_t OCCUPIED_CARRIERS_INC_DC = 601;

uint32_t get_long_cp_len(double sample_rate);
uint32_t get_short_cp_len(double sample_rate);
uint32_t get_fft_size(double sample_rate);

std::vector<std::complex<float>> create_zc_sequence(double sample_rate, uint32_t root);

#endif //GR_DRONEID_UTILS_H
