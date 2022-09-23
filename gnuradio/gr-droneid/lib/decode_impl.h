/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_DECODE_IMPL_H
#define INCLUDED_DRONEID_DECODE_IMPL_H

#include <gnuradio/droneid/decode.h>

namespace gr {
namespace droneid {

class decode_impl : public decode {
private:
    const std::string input_pdu_port_name_ = "pdus";
    const std::string output_pdu_port_name_ = "pdus";

    const std::string debug_path_;

    static std::vector<int8_t> qpsk_to_bits(const std::vector<gr_complex> & samples);
    // Nothing to declare in this block.

public:
    decode_impl(const std::string & debug_path);

    ~decode_impl();

    // Where all the action really happens
    int work(
        int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items
    );

    void handle_pdu(const pmt::pmt_t & pdu);
};

} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_DECODE_IMPL_H */
