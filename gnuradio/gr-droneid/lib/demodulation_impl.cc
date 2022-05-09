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
#include "demodulation_impl.h"
#include "droneid/misc_utils.h"
#include <volk/volk.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>

namespace gr {
    namespace droneid {
        using path = boost::filesystem::path;

        demodulation::sptr
        demodulation::make(double sample_rate, const std::string & debug_path) {
            return gnuradio::get_initial_sptr
                    (new demodulation_impl(sample_rate, debug_path));
        }


        /*
         * The private constructor
         */
        demodulation_impl::demodulation_impl(const double sample_rate, const std::string & debug_path)
                : gr::sync_block("demodulation",
                                 gr::io_signature::make(0, 0, 0),
                                 gr::io_signature::make(0, 0, 0)),
                                 sample_rate_(sample_rate), fft_size_(misc_utils::get_fft_size(sample_rate)), long_cp_len_(
                        misc_utils::get_long_cp_len(sample_rate)), short_cp_len_(misc_utils::get_short_cp_len(sample_rate)),
                        debug_path_(debug_path){
            if (! debug_path_.empty()) {
                const auto p = path(debug_path_);
                if (! boost::filesystem::is_directory(p)) {
                    boost::filesystem::create_directories(p);
                }
            }

            message_port_register_in(pmt::mp("pdus"));
            message_port_register_out(pmt::mp("pdus"));
            set_msg_handler(pmt::mp("pdus"), [this](pmt::pmt_t pdu){handle_msg(pdu);});
            cfo_cp_len_ = short_cp_len_;
            cfo_buffer_.resize(cfo_cp_len_ * 2);
//            sample_buffer_.resize((fft_size_ * 9) + (long_cp_len_ * 2) + (short_cp_len_ * 7));

            cp_lengths_ = {
                    long_cp_len_,
                    short_cp_len_,
                    short_cp_len_,
                    short_cp_len_,
                    short_cp_len_,
                    short_cp_len_,
                    short_cp_len_,
                    short_cp_len_,
                    long_cp_len_
            };

            symbols_.resize(cp_lengths_.size());
            for (auto & vec : symbols_) {
                vec.resize(fft_size_);
            }

            std::cout << "FFT SIZE: " << fft_size_ << ", sample rate: " << sample_rate_ << "\n";

            zc_ = misc_utils::create_zc_sequence(sample_rate_, 600);
            fft_ = std::unique_ptr<gr::fft::fft_complex>(new gr::fft::fft_complex(static_cast<int>(fft_size_), true, 1));
            fft_shift_ = std::unique_ptr<gr::fft::fft_shift<std::complex<float>>>(new gr::fft::fft_shift<std::complex<float>>(fft_size_));

            std::copy(zc_.begin(), zc_.end(), fft_->get_inbuf());
            fft_->execute();
            std::copy(fft_->get_outbuf(), fft_->get_outbuf() + fft_size_, zc_.begin());
            std::for_each(zc_.begin(), zc_.end(), [this](std::complex<float> & sample){ sample /= static_cast<float>(fft_size_);});
            fft_shift_->shift(zc_);
            channel_.resize(fft_size_);

            misc_utils::write_samples((path(debug_path_) / "zc").string(), zc_);
        }

        /*
         * Our virtual destructor.
         */
        demodulation_impl::~demodulation_impl() {
        }

        void demodulation_impl::handle_msg(pmt::pmt_t pdu) {
            const auto meta = pmt::car(pdu);
            const auto vec = pmt::cdr(pdu);

            const auto samples_ptr = pmt::c32vector_elements(vec, sample_count_);

            if (sample_buffer_.size() < sample_count_) {
                sample_buffer_.resize(sample_count_);
            }

            const auto start_idx_pmt = pmt::dict_ref(meta, pmt::mp("start_idx"), pmt::from_uint64(0));
            const auto start_idx = pmt::to_uint64(start_idx_pmt);

//            if (start_idx == 0) {
//                std::cerr << "Invalid start index!\n";
//                return;
//            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// CFO detection and adjustment using cyclic prefix
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            const auto cfo_symbol_offset_left = start_idx + fft_size_ + long_cp_len_;
            const auto * const left_cfo_window_ptr = samples_ptr + cfo_symbol_offset_left;
            const auto * const right_cfo_window_ptr = left_cfo_window_ptr + fft_size_;
            std::copy(left_cfo_window_ptr, left_cfo_window_ptr + cfo_cp_len_, &cfo_buffer_[0]);
            std::copy(right_cfo_window_ptr, right_cfo_window_ptr + cfo_cp_len_, &cfo_buffer_[cfo_cp_len_]);

            std::complex<float> dot_prod;
            volk_32fc_x2_conjugate_dot_prod_32fc(&dot_prod, &cfo_buffer_[0], &cfo_buffer_[cfo_cp_len_], cfo_cp_len_);
            const auto angle = std::arg(dot_prod) / static_cast<float>(fft_size_);

            std::cout << "Phase angle: " << angle << "\n";

            std::complex<float> phase = {1, 0};
            volk_32fc_s32fc_x2_rotator_32fc(&sample_buffer_[0], samples_ptr, -angle, &phase, sample_count_);

            const auto * symbol_ptr = samples_ptr;
            uint32_t offset = start_idx;
            for (size_t symbol_idx = 0; symbol_idx < cp_lengths_.size(); symbol_idx++) {
                offset += cp_lengths_[symbol_idx];
                std::copy(symbol_ptr + offset, symbol_ptr + offset + fft_size_, fft_->get_inbuf());
                fft_->execute();
                std::copy(fft_->get_outbuf(), fft_->get_outbuf() + fft_size_, symbols_[symbol_idx].begin());
                fft_shift_->shift(symbols_[symbol_idx]);
                offset += fft_size_;
                std::cout << "Offset: " << offset << " (" << (offset + fft_size_ + long_cp_len_) << ") / " << sample_count_ << "\n";
            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Channel estimation and equalization
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            volk_32fc_x2_divide_32fc(&channel_[0],  &zc_[0], &symbols_[3][0], fft_size_);
            misc_utils::write_samples((path(debug_path_) / "channel").string(), channel_);

            for (uint32_t symbol_idx = 0; symbol_idx < cp_lengths_.size(); symbol_idx++) {
                for (uint32_t idx = 0; idx < fft_size_; idx++) {

                }
//                volk_32fc_x2_multiply_32fc(&symbols_[symbol_idx][0], &symbols_[symbol_idx][0], &channel_[0], fft_size_);
            }

            message_port_pub(pmt::mp("pdus"), pmt::cons(pmt::make_dict(), pmt::init_c32vector(fft_size_ * 9, &symbols_[0][0])));
        }

        int
        demodulation_impl::work(int noutput_items,
                                gr_vector_const_void_star &input_items,
                                gr_vector_void_star &output_items) {
            // Do <+signal processing+>

            // Tell runtime system how many output items we produced.
            return noutput_items;
        }

    } /* namespace droneid */
} /* namespace gr */

