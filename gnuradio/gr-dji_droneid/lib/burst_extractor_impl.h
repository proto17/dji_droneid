/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DJI_DRONEID_BURST_EXTRACTOR_IMPL_H
#define INCLUDED_DJI_DRONEID_BURST_EXTRACTOR_IMPL_H

#include <gnuradio/dji_droneid/burst_extractor.h>

namespace gr {
namespace dji_droneid {

class burst_extractor_impl : public burst_extractor
{
private:
    const pmt::pmt_t output_port_name_;
    float threshold_;
    const pmt::pmt_t sample_tag_;

    pmt::pmt_t metadata_;
    uint64_t start_index_;
    std::vector<std::complex<float>> buffer_;
    std::complex<float> * buffer_ptr_;
    int32_t remaining_samples_;

    std::mutex settings_lock_;

    void publish_message();

public:
    burst_extractor_impl(float sample_rate, float threshold);
    ~burst_extractor_impl();

    void set_threshold(float threshold) override;

    pmt::pmt_t get_output_port_name() const override {
        return output_port_name_;
    }

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace dji_droneid
} // namespace gr

#endif /* INCLUDED_DJI_DRONEID_BURST_EXTRACTOR_IMPL_H */
