/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_DEMODULATION_IMPL_H
#define INCLUDED_DRONEID_DEMODULATION_IMPL_H

#include <gnuradio/droneid/demodulation.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>

namespace gr {
namespace droneid {

class demodulation_impl : public demodulation {
private:
    const double sample_rate_;
    const uint32_t fft_size_;
    const uint32_t long_cp_len_;
    const uint32_t short_cp_len_;
    const std::string debug_path_;

    const static uint8_t XOR_BIT_VEC [7200];

    std::unique_ptr<gr::fft::fft_complex_fwd> fft_;
    std::unique_ptr<gr::fft::fft_shift<std::complex<float>>> fft_shift_;
    uint32_t burst_counter_;
    size_t sample_count_;
    uint32_t cfo_cp_len_;
    std::vector<uint32_t> cp_lengths_;
    std::vector<std::complex<float>> zc_;
    std::vector<std::complex<float>> channel_;
    std::vector<std::vector<std::complex<float>>> symbols_;
    std::vector<std::complex<float>> cfo_buffer_;
    std::vector<std::complex<float>> sample_buffer_;

    void handle_msg(pmt::pmt_t pdu);
    // Nothing to declare in this block.

public:
    demodulation_impl(double sample_rate, const std::string & debug_path);

    ~demodulation_impl();

    // Where all the action really happens
    int work(
        int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items
    );
};

} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_DEMODULATION_IMPL_H */
