/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_IMPL_H
#define INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_IMPL_H

#include <gnuradio/droneid/normalized_xcorr_estimate.h>

namespace gr {
namespace droneid {

class normalized_xcorr_estimate_impl : public normalized_xcorr_estimate {
private:
    const uint32_t window_size_;
    float taps_var_;
    float window_size_recip_;
    gr_complex window_size_recip_complex_;
    std::vector<gr_complex> taps_;
    std::vector<gr_complex> sums_;
    std::vector<float> vars_;
    std::vector<float> abs_squared_;
    std::vector<gr_complex> buffer_;
    const gr_complex zero_complex_ = gr_complex{0, 0};
    // Nothing to declare in this block.

public:
    normalized_xcorr_estimate_impl(const std::vector<gr_complex> & taps);

    ~normalized_xcorr_estimate_impl();

    int general_work(int noutput_items,
                     gr_vector_int &ninput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items);

};


} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_IMPL_H */
