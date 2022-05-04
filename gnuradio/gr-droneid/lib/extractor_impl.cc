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
#include "extractor_impl.h"
#include "droneid/misc_utils.h"

namespace gr {
    namespace droneid {

        extractor::sptr
        extractor::make(double sample_rate) {
            return gnuradio::get_initial_sptr
                    (new extractor_impl(sample_rate));
        }


        /*
         * The private constructor
         *
         * TODO(3May2022): Should maybe make this a sink block since it doesn't produce samples?
         */
        extractor_impl::extractor_impl(double sample_rate)
                : gr::sync_block("extractor",
                                 gr::io_signature::make2(2, 2, sizeof(gr_complex), sizeof(float)),
                                 gr::io_signature::make(0, 0, 0)), fft_size_(misc_utils::get_fft_size(sample_rate)),
                                 long_cp_len_(misc_utils::get_long_cp_len(sample_rate)), short_cp_len_(misc_utils::get_short_cp_len(sample_rate)),
                                 // This is more samples than need to be extracted, but it's nice to add some padding to the end
                                 extract_samples_count_((fft_size_ * 11) + (long_cp_len_ * 2) + (short_cp_len_ * 7)){
            this->message_port_register_out(pmt::mp("pdus"));

            buffer_.resize(extract_samples_count_);
            current_state_ = state_t::WAITING_FOR_THRESHOLD;
            collected_samples_ = 0;
            total_samples_read_ = 0;
        }

        /*
         * Our virtual destructor.
         */
        extractor_impl::~extractor_impl() {
        }

        int
        extractor_impl::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) {
            // The first input stream is the complex samples delayed by however many samples the ZC sequence is into the
            // burst.  Example: if the FFT size is 1024, then the short cyclic prefix is 72 and long cyclic prefix is
            // 80 samples.  Since the ZC sequence is the 4th symbol, the delay needs to be at least:
            //   (1024 * 4) + 80 + (72 * 4)
            // This could probably be done with set_history(), but making the graph do it is pretty easy too
            const auto *samples = (const gr_complex *) input_items[0];

            // The second input stream is the output of the cross correlation for the first ZC sequence with no delay
            // These values are the abs squared scores from the cross correlation output
            const auto *correlation_scores = (const float *) input_items[1];

            // Walk over each sample and take the appropriate action depending on the current state
            // There is a MUCH faster way to do this!  When a new burst is found the burst extractor should memcpy as
            // many samples as it can/needs from the current buffer vs pulling the samples out one at a time.  But,
            // that's a task for later.  The logic below is slow, but does work
            for (int idx = 0; idx < noutput_items; idx++) {
                total_samples_read_++;
                switch (current_state_) {
                    // In this state the logic is looking for a correlation score that is >= to the threshold value.
                    // If found, it saves off the current sample, marks how many samples need to be read in, saves off
                    // the sample index the threshold was passed, and moves the state machine to COLLECTING_SAMPLES
                    case state_t::WAITING_FOR_THRESHOLD: {
                        if (correlation_scores[idx] > 0.5) {
                            start_sample_index_ = nitems_read(0) + idx;
                            std::cout << "Found burst @ " << total_samples_read_ << " / " << start_sample_index_ << "\n";
                            current_state_ = state_t::COLLECTING_SAMPLES;
                            collected_samples_ = 1;
                            buffer_[0] = samples[idx];
                        }
                        break;
                    }

                    // Stay in this state for as long as there are samples remaining to collect.  Once all of the
                    // samples have been collected, write out a PDU containing the collected samples and create a
                    // dictionary that contains the starting sample offset.  This value isn't actually important for
                    // processing, but nice to have for tracking time between bursts and maybe for some logic that
                    // can move the radio to a new frequency if no frames have been seen in X samples
                    case state_t::COLLECTING_SAMPLES: {
                        buffer_[collected_samples_++] = samples[idx];
                        if (collected_samples_ == extract_samples_count_) {
                            current_state_ = state_t::WAITING_FOR_THRESHOLD;
                            const pmt::pmt_t vec = pmt::init_c32vector(buffer_.size(), buffer_);
                            pmt::pmt_t meta = pmt::make_dict();
                            meta = pmt::dict_add(meta, pmt::mp("start_offset"), pmt::from_uint64(start_sample_index_));
                            std::cout << "Sending message with " << pmt::length(vec) << " elements (" << extract_samples_count_ << ") starting at " << start_sample_index_ << "\n";
                            message_port_pub(pmt::mp("pdus"), pmt::cons(meta, vec));

                            collected_samples_ = 0;
                        }
                        break;
                    }
                }
            }

            // Tell runtime system how many output items we produced.
            return noutput_items;
        }

    } /* namespace droneid */
} /* namespace gr */

