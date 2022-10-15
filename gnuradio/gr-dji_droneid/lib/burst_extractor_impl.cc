/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "burst_extractor_impl.h"
#include <gnuradio/io_signature.h>

#include <gnuradio/dji_droneid/utils.h>

namespace gr {
namespace dji_droneid {

burst_extractor::sptr burst_extractor::make(const float sample_rate, const float threshold)
{
    return gnuradio::make_block_sptr<burst_extractor_impl>(sample_rate, threshold);
}


/*
 * The private constructor
 */
burst_extractor_impl::burst_extractor_impl(const float sample_rate, const float threshold)
    : gr::sync_block("burst_extractor",
                     gr::io_signature::make2(
                         2, 2, sizeof(std::complex<float>), sizeof(float)),
                     gr::io_signature::make(
                         0, 0, 0)),
      output_port_name_(pmt::mp("bursts")), threshold_(threshold), sample_tag_(pmt::mp("start_index"))
{
    message_port_register_out(output_port_name_);

    const auto fft_size = utils::get_fft_size(sample_rate);
    const auto [long_cp_len, short_cp_len] = utils::get_cyclic_prefix_lengths(sample_rate);

    const auto full_frame_length = long_cp_len + (short_cp_len * 8) + (fft_size * 9);
    const auto extract_size = full_frame_length + (fft_size * 2);

    buffer_.resize(extract_size);
    remaining_samples_ = -1;
    buffer_ptr_ = nullptr;

    metadata_ = pmt::make_dict();
}

/*
 * Our virtual destructor.
 */
burst_extractor_impl::~burst_extractor_impl() {}

int burst_extractor_impl::work(int noutput_items,
                               gr_vector_const_void_star& input_items,
                               gr_vector_void_star& output_items)
{
    std::lock_guard<decltype(settings_lock_)> l(settings_lock_);

    auto in_samples = static_cast<const std::complex<float> *>(input_items[0]);
    auto in_scores = static_cast<const float *>(input_items[1]);

    // This is HORRIBLY inefficient, but works for now.  A much better way would be to copy out chunks of
    // samples at a time after the threshold has been passed, but that requires effort.

    // Walk through each sample of the input either looking for the threshold to be passed, or accumulating
    // samples in the buffer until all samples have been read
    for (auto idx = decltype(noutput_items){0}; idx < noutput_items; idx++) {
        // First step is for when no burst has yet been detected.  Check if the threshold has been met, and
        // if so then start buffering up samples
        if (remaining_samples_ == -1) {
            if (in_scores[idx] >= threshold_) {
                remaining_samples_ = static_cast<int32_t>(buffer_.size());
                buffer_[0] = in_samples[idx];
                buffer_ptr_ = &buffer_[1];
                start_index_ = nitems_read(0) + idx;
            }
        // This state is for the last sample that's read for a burst
        } else if (remaining_samples_ == 1) {
            *buffer_ptr_++ = in_samples[idx];
            publish_message();
            remaining_samples_ = -1;
            buffer_ptr_ = nullptr;
        // Finally, this state is for when there are still more samples to read for a burst
        } else {
            *buffer_ptr_++ = in_samples[idx];
            remaining_samples_--;
        }
    }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

void burst_extractor_impl::publish_message() {
    // Update the metadata with the current burst's start index
    metadata_ = pmt::dict_add(metadata_, sample_tag_, pmt::from_uint64(start_index_));

    // Build a PDU pair and send it
    const auto pdu = pmt::cons(
        metadata_, pmt::init_c32vector(buffer_.size(), buffer_));
    message_port_pub(output_port_name_, pdu);
}

void burst_extractor_impl::set_threshold(const float threshold) {
    std::lock_guard<decltype(settings_lock_)> l(settings_lock_);
    threshold_ = threshold;
}

} /* namespace dji_droneid */
} /* namespace gr */
