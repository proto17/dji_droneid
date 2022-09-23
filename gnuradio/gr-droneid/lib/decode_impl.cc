/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "decode_impl.h"
#include <gnuradio/io_signature.h>
extern "C" {
#include <turbofec/rate_match.h>
#include <turbofec/turbo.h>
}

#include <gnuradio/io_signature.h>
#include "decode_impl.h"
#include <boost/filesystem.hpp>
#include <gnuradio/droneid/misc_utils.h>
#include <chrono>

// This line is required before the include of CRC.h so that it actually loads the CRC_24_LTEA definition
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
#include <CRC.h>

namespace gr {
namespace droneid {

using path = boost::filesystem::path;

decode::sptr
decode::make(const std::string & debug_path) {
    return gnuradio::get_initial_sptr
        (new decode_impl(debug_path));
}


/*
         * The private constructor
 */
decode_impl::decode_impl(const std::string & debug_path)
    : gr::sync_block("decode",
                     gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(0, 0, 0)),
      debug_path_(debug_path){
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
    const auto start_time = std::chrono::high_resolution_clock::now();
    const pmt::pmt_t meta = pmt::car(pdu);
    const pmt::pmt_t vec = pmt::cdr(pdu);

    const auto samples = pmt::c32vector_elements(vec);

    if (samples.size() != 3600) {
        std::cout << "Invalid number of samples.  Expected 3600, got " << samples.size() << "\n";
        return;
    }

    auto bits = qpsk_to_bits(samples);

    if (! debug_path_.empty()) {
        misc_utils::write((path(debug_path_) / "bits").string(), &bits[0], sizeof(bits[0]), bits.size());
        const auto bit_string = misc_utils::bit_vec_to_string(bits);
        misc_utils::write((path(debug_path_) / "bit_string").string(), &bit_string[0], sizeof(bit_string[0]), bit_string.size());
    }
    //            misc_utils::print_bits(bits);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Setup and run the Turbo decoder
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const size_t input_file_bit_count = 7200;

    const int turbo_iterations = 4;
    const int turbo_decoder_bit_count = 1412;  // Number of bits that the Turbo decoder will take in
    const int expected_payload_bytes = 176;    // Number of bytes that the Turbo decoder will output
    const int expected_payload_bits = expected_payload_bytes * 8;

    // Allocate buffers for the Turbo decoder
    std::vector<int8_t> d1(turbo_decoder_bit_count);
    std::vector<int8_t> d2(turbo_decoder_bit_count);
    std::vector<int8_t> d3(turbo_decoder_bit_count);
    std::vector<uint8_t> decoded_bytes(expected_payload_bytes);

    // Create the required structures to run the Turbo decoder
    struct lte_rate_matcher * rate_matcher = lte_rate_matcher_alloc();
    struct tdecoder * turbo_decoder = alloc_tdec();
    struct lte_rate_matcher_io rate_matcher_io = {
        .D = turbo_decoder_bit_count,
        .E = input_file_bit_count,
        .d = {&d1[0], &d2[0], &d3[0]},
        .e = &bits[0]
    };

    // Setup the rate matching logic
    lte_rate_match_rv(rate_matcher, &rate_matcher_io, 0);

    // Run the turbo decoder (will do rate matching as well)
    const int decode_status = lte_turbo_decode(turbo_decoder, expected_payload_bits, turbo_iterations,
                                               &decoded_bytes[0], &d1[0], &d2[0], &d3[0]);

    if (decode_status != 0) {
        std::cerr << "Failed to decode\n";
    } else {
        for (const auto & b : decoded_bytes) {
            fprintf(stdout, "%02x", b);
        }
        fprintf(stdout, "\n");

        const auto crc_out = CRC::Calculate(&decoded_bytes[0], decoded_bytes.size(), CRC::CRC_24_LTEA());
        if (crc_out != 0) {
            std::cerr << "CRC Check Failed!\n";
        }

        message_port_pub(pmt::mp(output_pdu_port_name_), pmt::cons(pmt::make_dict(), pmt::init_u8vector(decoded_bytes.size(), decoded_bytes)));
    }

    free_tdec(turbo_decoder);
    lte_rate_matcher_free(rate_matcher);

    const auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "Time: " << std::chrono::duration<float>(end_time - start_time).count() << "\n";
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