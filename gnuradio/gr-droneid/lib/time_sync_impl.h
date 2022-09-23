/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_TIME_SYNC_IMPL_H
#define INCLUDED_DRONEID_TIME_SYNC_IMPL_H

#include <gnuradio/droneid/time_sync.h>

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
    const std::string debug_path_;

    size_t pdu_element_count_;
    std::vector<std::complex<float>> buffer_;
    std::vector<float> mags_;
    std::unique_ptr<filter_t> correlator_ptr_;
    uint32_t file_counter_ = 0;
    void msg_handler(const pmt::pmt_t & pdu);

public:
    time_sync_impl(double sample_rate, const std::string & debug_path);

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
