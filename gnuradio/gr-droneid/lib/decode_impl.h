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

#ifndef INCLUDED_DRONEID_DECODE_IMPL_H
#define INCLUDED_DRONEID_DECODE_IMPL_H

#include <vector>

#include <droneid/decode.h>

#include "droneid/lte_decode.h"

namespace gr {
    namespace droneid {

        class decode_impl : public decode {
        private:
            std::unique_ptr<lte_decode> decoder_ptr_;
            const std::string input_pdu_port_name_ = "pdus";
            const std::string output_pdu_port_name_ = "pdus";

            std::vector<int8_t> qpsk_to_bits(const std::vector<gr_complex> & samples);
            // Nothing to declare in this block.

        public:
            decode_impl();

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

