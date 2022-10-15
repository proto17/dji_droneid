/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "detector_impl.h"
#include <gnuradio/io_signature.h>

#include <gnuradio/dji_droneid/utils.h>


namespace gr {
namespace dji_droneid {

detector::sptr detector::make(float sample_rate)
{
    return gnuradio::make_block_sptr<detector_impl>(sample_rate);
}


/*
 * The private constructor
 */
detector_impl::detector_impl(const float sample_rate)
    : gr::hier_block2("detector",
                      gr::io_signature::make(
                          1 /* min inputs */, 1 /* max inputs */, sizeof(gr_complex)),
                      gr::io_signature::make(
                          1 /* min outputs */, 1 /*max outputs */, sizeof(float)))
{
    const auto fft_size = utils::get_fft_size(sample_rate);
    const auto zc = utils::create_zc(fft_size, 4);
    const auto zc_variance = utils::variance_vector(zc);

    corr_filter_ = gr::filter::fft_filter_ccc::make(1, utils::conj_vector(zc));

    c2mag_sq_ = gr::blocks::complex_to_mag_squared::make();
    moving_avg_ = gr::blocks::moving_average_ff::make(static_cast<int32_t>(fft_size), 1.0f / static_cast<float>(fft_size - 1));
    mult_const_ff_ = gr::blocks::multiply_const_ff::make(zc_variance);
    transcendental_ = gr::blocks::transcendental::make("sqrt");
    float_to_complex_ = gr::blocks::float_to_complex::make();
    const_source_ = gr::analog::sig_source_f::make(0, gr::analog::GR_CONST_WAVE, 0, 0, 0);

    mult_const_cc_ = gr::blocks::multiply_const_cc::make({1.0f / static_cast<float>(fft_size), 0});
    divide_ = gr::blocks::divide_cc::make();
    c2mag_ = gr::blocks::complex_to_mag::make();

    connect(self(), 0, corr_filter_, 0);
    connect(self(), 0, c2mag_sq_, 0);
    connect(corr_filter_, 0, mult_const_cc_, 0);
    connect(c2mag_sq_, 0, moving_avg_, 0);
    connect(moving_avg_, 0, mult_const_ff_, 0);
    connect(mult_const_ff_, 0, transcendental_, 0);
    connect(transcendental_, 0, float_to_complex_, 0);
    connect(const_source_, 0, float_to_complex_, 1);
    connect(mult_const_cc_, 0, divide_, 0);
    connect(float_to_complex_, 0, divide_, 1);
    connect(divide_, 0, c2mag_, 0);
    connect(c2mag_, 0, self(), 0);
}

/*
 * Our virtual destructor.
 */
detector_impl::~detector_impl() {}


} /* namespace dji_droneid */
} /* namespace gr */
