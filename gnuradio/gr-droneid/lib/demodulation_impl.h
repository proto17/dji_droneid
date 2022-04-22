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

#ifndef INCLUDED_DRONEID_DEMODULATION_IMPL_H
#define INCLUDED_DRONEID_DEMODULATION_IMPL_H

#include <droneid/demodulation.h>
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
            void write_samples(const std::string &path, const std::complex<float> * samples, uint32_t element_count);
            void write_samples(const std::string & path, const std::vector<std::complex<float>> & samples);

            std::unique_ptr<gr::fft::fft_complex> fft_;
            std::unique_ptr<gr::fft::fft_shift<std::complex<float>>> fft_shift_;
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
            demodulation_impl(double sample_rate);

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

