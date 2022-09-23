/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extractor_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/droneid/misc_utils.h>

namespace gr {
namespace droneid {

extractor::sptr
extractor::make(const double sample_rate, const float threshold) {
    return gnuradio::get_initial_sptr
        (new extractor_impl(sample_rate, threshold));
}


/*
         * The private constructor
 */
extractor_impl::extractor_impl(double sample_rate, const float threshold)
    : gr::sync_block("extractor",
                     gr::io_signature::make2(2, 2, sizeof(gr_complex), sizeof(float)),
                     gr::io_signature::make(0, 0, 0)), fft_size_(misc_utils::get_fft_size(sample_rate)),
      long_cp_len_(misc_utils::get_long_cp_len(sample_rate)), short_cp_len_(misc_utils::get_short_cp_len(sample_rate)),
      extract_samples_count_((fft_size_ * 13) + (long_cp_len_ * 2) + (short_cp_len_ * 7)){
    this->message_port_register_out(pmt::mp("pdus"));

    buffer_.resize(extract_samples_count_);
    current_state_ = state_t::WAITING_FOR_THRESHOLD;
    collected_samples_ = 0;
    total_samples_read_ = 0;
    threshold_ = threshold;
}

/*
         * Our virtual destructor.
 */
extractor_impl::~extractor_impl() {
}

void extractor_impl::set_threshold(const float threshold) {
    std::lock_guard<decltype(parameter_lock_)> l(parameter_lock_);
    threshold_ = threshold;
}

int
extractor_impl::work(int noutput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items) {
    const gr_complex *samples = (const gr_complex *) input_items[0];
    const float *correlation_scores = (const float *) input_items[1];

    float threshold;
    {
        std::lock_guard<decltype(parameter_lock_)> lock(parameter_lock_);
        threshold = threshold_;
    }

    for (int idx = 0; idx < noutput_items; idx++) {
        total_samples_read_++;
        switch (current_state_) {
        case state_t::WAITING_FOR_THRESHOLD: {
            if (correlation_scores[idx] > threshold) {
                start_sample_index_ = nitems_read(0) + idx;
                std::cout << "Found burst @ " << total_samples_read_ << " / " << start_sample_index_ << "\n";
                current_state_ = state_t::COLLECTING_SAMPLES;
                collected_samples_ = 1;
                buffer_[0] = samples[idx];
            }
            break;
        }

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
    // Do <+signal processing+>

    // Tell runtime system how many output items we produced.
    return noutput_items;
}


} /* namespace droneid */
} /* namespace gr */
