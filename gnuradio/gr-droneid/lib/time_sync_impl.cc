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
#include "time_sync_impl.h"
#include <volk/volk.h>

#include "droneid/misc_utils.h"
#include <gnuradio/filter/fir_filter.h>

namespace gr {
    namespace droneid {

        time_sync::sptr
        time_sync::make(double sample_rate) {
            return gnuradio::get_initial_sptr
                    (new time_sync_impl(sample_rate));
        }


        /*
         * The private constructor
         */
        time_sync_impl::time_sync_impl(double sample_rate)
                : gr::sync_block("time_sync",
                                 gr::io_signature::make(0, 0, 0),
                                 gr::io_signature::make(0, 0, 0)),
                  sample_rate_(sample_rate),
                  fft_size_(misc_utils::get_fft_size(sample_rate)), long_cp_len_(misc_utils::get_long_cp_len(sample_rate)),
                  short_cp_len_(misc_utils::get_short_cp_len(sample_rate)) {
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
            auto burst_ptr = pmt::c32vector_elements(vec, pdu_element_count_);
            misc_utils::write_samples("/tmp/bursts/file_"+std::to_string(file_counter_), burst_ptr, pdu_element_count_);

            if (buffer_.size() < pdu_element_count_) {
                buffer_.resize(pdu_element_count_);
                mags_.resize(pdu_element_count_);
            }

            // Run the correlator ignoring the last `ntaps` elements so as not to walk off the end of the array
            correlator_ptr_->filterN(&buffer_[0], burst_ptr, pdu_element_count_ - correlator_ptr_->ntaps());

            misc_utils::write_samples("/tmp/bursts/corr_file_"+std::to_string(file_counter_), buffer_);

            // Find the max correlation peak
            // TODO: Only search through the area where the ZC is expected to be
            volk_32fc_magnitude_squared_32f(&mags_[0], &buffer_[0], buffer_.size());
            uint16_t max_idx;
            volk_32f_index_max_16u(&max_idx, &mags_[0], mags_.size());

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

