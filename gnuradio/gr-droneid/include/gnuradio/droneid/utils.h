/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_UTILS_H
#define INCLUDED_DRONEID_UTILS_H

#include <gnuradio/droneid/api.h>


#include <cstdint>
#include <vector>
#include <complex>

namespace gr {
namespace droneid {

/*!
 * \brief <+description+>
 *
 */
class DRONEID_API utils
{
public:
    utils();
    ~utils();

    static constexpr double CARRIER_SPACING = 15e3;
    static constexpr unsigned int OCCUPIED_CARRIERS_EXC_DC = 600;
    static constexpr unsigned int OCCUPIED_CARRIERS_INC_DC = 601;

    static unsigned int get_long_cp_len(double sample_rate);
    static unsigned int get_short_cp_len(double sample_rate);
    static unsigned int get_fft_size(double sample_rate);

    static std::vector<std::complex<float>> create_zc_sequence(double sample_rate, uint32_t root);
private:
};



} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_UTILS_H */
