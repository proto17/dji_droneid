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

#ifndef INCLUDED_DRONEID_EXTRACTOR_IMPL_H
#define INCLUDED_DRONEID_EXTRACTOR_IMPL_H

#include <droneid/extractor.h>

namespace gr {
    namespace droneid {

        class extractor_impl : public extractor {
        private:
            enum class state_t {
                WAITING_FOR_THRESHOLD,
                COLLECTING_SAMPLES
            };


            static constexpr float CARRIER_SPACING = 15e3;
            uint64_t start_sample_index_;
            uint64_t total_samples_read_;
            state_t current_state_;
            uint32_t collected_samples_;
            const uint32_t fft_size_;
            const uint32_t long_cp_len_;
            const uint32_t short_cp_len_;
            const uint32_t extract_samples_count_;
            std::vector<gr_complex> buffer_;

            float threshold_;
            std::mutex parameter_lock_;

        public:
            extractor_impl(double sample_rate, float threshold);

            ~extractor_impl();

            void set_threshold(float threshold) override;

            // Where all the action really happens
            int work(
                    int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items
            );
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_EXTRACTOR_IMPL_H */

