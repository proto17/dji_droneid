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
#include "normalized_xcorr_estimate_impl.h"

#include <volk/volk.h>

namespace gr {
    namespace droneid {

        normalized_xcorr_estimate::sptr
        normalized_xcorr_estimate::make(std::vector<std::complex<float>> filter_taps) {
            return gnuradio::get_initial_sptr
                    (new normalized_xcorr_estimate_impl(filter_taps));
        }


        /*
         * The private constructor
         */
        normalized_xcorr_estimate_impl::normalized_xcorr_estimate_impl(std::vector<std::complex<float>> filter_taps)
                : gr::sync_block("normalized_xcorr_estimate",
                                 gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                 gr::io_signature::make(1, 1, sizeof(float))) {
            xcorr_ = std::unique_ptr<normalized_xcorr>(new normalized_xcorr(filter_taps));

            set_history(filter_taps.size());
            set_alignment(std::max(1, static_cast<int32_t>(volk_get_alignment() / sizeof(float))));
        }

        /*
         * Our virtual destructor.
         */
        normalized_xcorr_estimate_impl::~normalized_xcorr_estimate_impl() {
        }

        int
        normalized_xcorr_estimate_impl::work(int noutput_items,
                                             gr_vector_const_void_star &input_items,
                                             gr_vector_void_star &output_items) {
            const auto *in = (const gr_complex *) input_items[0];
            auto *out = (float *) output_items[0];

            xcorr_->run(in, noutput_items, out);

            // Do <+signal processing+>

            // Tell runtime system how many output items we produced.
            return noutput_items;
        }

    } /* namespace droneid */
} /* namespace gr */

