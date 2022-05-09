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
#include "decode_impl.h"

#include "droneid/misc_utils.h"

namespace gr {
    namespace droneid {

        decode::sptr
        decode::make() {
            return gnuradio::get_initial_sptr
                    (new decode_impl());
        }


        /*
         * The private constructor
         */
        decode_impl::decode_impl()
                : gr::sync_block("decode",
                                 gr::io_signature::make(0, 0, 0),
                                 gr::io_signature::make(0, 0, 0)) {
            decoder_ptr_ = std::unique_ptr<lte_decode>(new lte_decode());

            message_port_register_in(pmt::mp(input_pdu_port_name_));
            message_port_register_out(pmt::mp(output_pdu_port_name_));

            set_msg_handler(pmt::mp(input_pdu_port_name_), [this](const pmt::pmt_t & pdu){
                this->handle_pdu(pdu);
            });
        }

        /*
         * Our virtual destructor.
         */
        decode_impl::~decode_impl() {
        }

        std::vector<int8_t> decode_impl::qpsk_to_bits(const std::vector<gr_complex> &samples) {
            std::vector<int8_t> bits(samples.size() * 2);

            auto * bit_vec_ptr = &bits[0];

            std::for_each(samples.begin(), samples.end(), [&bit_vec_ptr](const gr_complex & sample) {
                if (sample.real() > 0 && sample.imag() > 0) {
                    *bit_vec_ptr++ = 0;
                    *bit_vec_ptr++ = 0;
                } else if (sample.real() > 0 && sample.imag() < 0) {
                    *bit_vec_ptr++ = 0;
                    *bit_vec_ptr++ = 1;
                } else if (sample.real() < 0 && sample.imag() > 0) {
                    *bit_vec_ptr++ = 1;
                    *bit_vec_ptr++ = 0;
                } else {
                    *bit_vec_ptr++ = 1;
                    *bit_vec_ptr++ = 1;
                }
            });

            return bits;
        }

        void decode_impl::handle_pdu(const pmt::pmt_t & pdu) {
            const pmt::pmt_t meta = pmt::car(pdu);
            const pmt::pmt_t vec = pmt::cdr(pdu);

            const auto samples = pmt::c32vector_elements(vec);

            if (samples.size() != 3600) {
                std::cout << "Invalid number of samples.  Expected 3600, got " << samples.size() << "\n";
                return;
            }

            const auto bits = qpsk_to_bits(samples);
            misc_utils::print_bits(bits);

            const auto bytes = decoder_ptr_->decode(bits);

            for (const auto & b : bytes) {
                fprintf(stdout, "%02x", b);
            }
            fprintf(stdout, "\n");
        }

        int
        decode_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items) {

            // Do <+signal processing+>

            // Tell runtime system how many output items we produced.
            return noutput_items;
        }

    } /* namespace droneid */
} /* namespace gr */

