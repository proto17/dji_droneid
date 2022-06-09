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

#include <boost/filesystem.hpp>

#include <gnuradio/io_signature.h>
#include "time_sync_impl.h"
#include <volk/volk.h>

#include "droneid/misc_utils.h"
#include <gnuradio/filter/fir_filter.h>

namespace gr {
    namespace droneid {

        using path = boost::filesystem::path;

        time_sync::sptr
        time_sync::make(double sample_rate, const std::string & debug_path) {
            return gnuradio::get_initial_sptr
                    (new time_sync_impl(sample_rate, debug_path));
        }


        /*
         * The private constructor
         */
        time_sync_impl::time_sync_impl(double sample_rate, const std::string & debug_path)
                : gr::sync_block("time_sync",
                                 gr::io_signature::make(0, 0, 0),
                                 gr::io_signature::make(0, 0, 0)),
                  sample_rate_(sample_rate),
                  fft_size_(misc_utils::get_fft_size(sample_rate)), long_cp_len_(misc_utils::get_long_cp_len(sample_rate)),
                  short_cp_len_(misc_utils::get_short_cp_len(sample_rate)), debug_path_(debug_path) {
            if (! debug_path_.empty()) {
                const auto p = path(debug_path_);
                if (! boost::filesystem::is_directory(p)) {
                    boost::filesystem::create_directories(p);
                }
            }

            auto zc_sequence = misc_utils::create_zc_sequence(sample_rate_, 600);
            std::for_each(zc_sequence.begin(), zc_sequence.end(), [](std::complex<float> & sample){ sample = std::conj(sample); });
            std::reverse(zc_sequence.begin(), zc_sequence.end());
            correlator_ptr_ = std::unique_ptr<filter_t>(new filter_t(1, zc_sequence));

            message_port_register_in(pmt::mp("pdus"));
            message_port_register_out(pmt::mp("pdus"));
            set_msg_handler(pmt::mp("pdus"), [this](pmt::pmt_t pdu) { this->msg_handler(pdu); });
        }

        /*
         * Our virtual destructor.
         */
        time_sync_impl::~time_sync_impl() {
        }

        void time_sync_impl::msg_handler(const pmt::pmt_t & pdu) {
            auto meta = pmt::car(pdu);
            auto vec = pmt::cdr(pdu);
            const auto samples = pmt::c32vector_elements(vec);
            if (! debug_path_.empty()) {
                misc_utils::write_samples((path(debug_path_) / std::to_string(file_counter_++)).string(), samples);
            }

            if (buffer_.size() < samples.size()) {
                buffer_.resize(samples.size());
                mags_.resize(samples.size());
            }

            const auto max_idx = misc_utils::find_zc_seq_start_idx(samples, sample_rate_);
            const uint32_t offset = (fft_size_ * 3) + (short_cp_len_ * 3) + long_cp_len_;

            std::cout << "Max: " << max_idx << " offset: " << offset << " - " << (max_idx - offset) << "\n";
            std::cout << "short: " << short_cp_len_ << " long: " << long_cp_len_ << " fft: " << fft_size_ << "\n";

            auto new_meta = pmt::make_dict();
            new_meta = pmt::dict_add(new_meta, pmt::mp("start_idx"), pmt::from_uint64(max_idx - offset));
            pmt::print(new_meta);
            message_port_pub(pmt::mp("pdus"), pmt::cons(new_meta, vec));

            file_counter_++;
        }

        int
        time_sync_impl::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) {
            // Tell runtime system how many output items we produced.
            return noutput_items;
        }

    } /* namespace droneid */
} /* namespace gr */

