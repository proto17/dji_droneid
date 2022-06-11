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

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_normalized_xcorr.h"
#include <droneid/normalized_xcorr.h>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <gnuradio/random.h>
#include <chrono>

#include <droneid/misc_utils.h>

namespace gr {
  namespace droneid {
      using sample_t = float;
      using complex_t = std::complex<sample_t>;
      using complex_vec_t = std::vector<complex_t>;

    BOOST_AUTO_TEST_CASE(foo) {
        auto rng = gr::random();
        complex_vec_t noise_vector(1e6, {0, 0});
        for (auto & sample : noise_vector) {
            sample = rng.rayleigh_complex();
        }

        const auto start_offset = 102313; // Just a random starting offset that's not on an even boundary
        const auto filter_length = 1023;

        const auto filter_taps = complex_vec_t(noise_vector.begin() + start_offset, noise_vector.begin() + start_offset + filter_length);


        normalized_xcorr xcorr(filter_taps);

        std::vector<sample_t> mags(noise_vector.size() - filter_length, 0);

        const auto start = std::chrono::high_resolution_clock::now();
        xcorr.run(&noise_vector[0], noise_vector.size(), &mags[0]);
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration<float>(end - start).count();
        const auto rate = noise_vector.size() / duration;
        std::cout << "Took " << duration << " seconds to run through " << noise_vector.size() << " samples\n";
        std::cout << "Average of " << rate << " samples per second\n";


        misc_utils::write("/tmp/mags", &mags[0], sizeof(mags[0]), mags.size());

        const auto max_element_iter = std::max_element(mags.begin(), mags.end());
        const auto distance = std::distance(mags.begin(), max_element_iter);
        std::cout << "Distance: " << distance << "\n";
    }


  } /* namespace droneid */
} /* namespace gr */

