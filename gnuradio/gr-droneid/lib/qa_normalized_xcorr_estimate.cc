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

#include <iostream>

#include <gnuradio/attributes.h>
#include "qa_normalized_xcorr_estimate.h"

#include <droneid/normalized_xcorr_estimate.h>
#include <droneid/misc_utils.h>

#include <boost/test/unit_test.hpp>

#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/top_block.h>

namespace gr {
    namespace droneid {
        BOOST_AUTO_TEST_CASE(normalized_xcorr_estimate_test) {
            auto tb = gr::make_top_block("top");

            const auto noise = misc_utils::create_gaussian_noise(12000);
            const auto taps_offset = 4562;
            const auto taps_size = 1024;
            const auto taps = std::vector<gr_complex>(noise.begin() + taps_offset, noise.begin() + taps_offset + taps_size);

            auto source = gr::blocks::vector_source<gr_complex>::make(noise);
            auto sink = gr::blocks::vector_sink<gr_complex>::make();

            auto uut = droneid::normalized_xcorr_estimate::make(taps);

            tb->connect(source, 0, uut, 0);
            tb->connect(uut, 0, sink, 0);

            tb->run();

            std::cout << "Sent in " << noise.size() << " samples, got back " << sink->data().size() << " samples\n";
        }
    } /* namespace droneid */
} /* namespace gr */

