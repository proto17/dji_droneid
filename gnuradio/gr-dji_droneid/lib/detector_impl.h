/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DJI_DRONEID_DETECTOR_IMPL_H
#define INCLUDED_DJI_DRONEID_DETECTOR_IMPL_H

#include <gnuradio/dji_droneid/detector.h>

#include <gnuradio/blocks/complex_to_mag_squared.h>
#include <gnuradio/blocks/moving_average.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/transcendental.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/divide.h>
#include <gnuradio/blocks/complex_to_mag.h>
#include <gnuradio/filter/fft_filter_ccc.h>

namespace gr {
namespace dji_droneid {

class detector_impl : public detector
{
private:
    gr::blocks::complex_to_mag_squared::sptr c2mag_sq_;
    gr::blocks::moving_average_ff::sptr moving_avg_;
    gr::blocks::multiply_const_ff::sptr mult_const_ff_;
    gr::blocks::transcendental::sptr transcendental_;
    gr::blocks::float_to_complex::sptr float_to_complex_;
    gr::analog::sig_source_f::sptr const_source_;
    gr::blocks::divide_cc::sptr divide_;
    gr::blocks::complex_to_mag::sptr c2mag_;
    gr::blocks::multiply_const_cc::sptr mult_const_cc_;
    gr::filter::fft_filter_ccc::sptr corr_filter_;

public:
    detector_impl(float sample_rate);
    ~detector_impl();

    // Where all the action really happens
};

} // namespace dji_droneid
} // namespace gr

#endif /* INCLUDED_DJI_DRONEID_DETECTOR_IMPL_H */
