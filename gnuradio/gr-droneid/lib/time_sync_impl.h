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

#ifndef INCLUDED_DRONEID_TIME_SYNC_IMPL_H
#define INCLUDED_DRONEID_TIME_SYNC_IMPL_H

#include <droneid/time_sync.h>

#include <gnuradio/filter/fir_filter.h>

namespace gr {
    namespace droneid {

        class time_sync_impl : public time_sync {
        private:
            using filter_t = gr::filter::kernel::fir_filter_ccc;
            const double sample_rate_;
            const uint32_t fft_size_;
            const uint32_t long_cp_len_;
            const uint32_t short_cp_len_;

            size_t pdu_element_count_;
            std::vector<std::complex<float>> buffer_;
            std::vector<float> mags_;
            std::unique_ptr<filter_t> correlator_ptr_;
            uint32_t file_counter_ = 0;
            void msg_handler(const pmt::pmt_t & pdu);

        public:
            time_sync_impl(double sample_rate);

            ~time_sync_impl();

            // Where all the action really happens
            int work(
                    int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items
            );
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_TIME_SYNC_IMPL_H */

