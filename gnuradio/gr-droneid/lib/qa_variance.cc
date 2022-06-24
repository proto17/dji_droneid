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
#include "qa_variance.h"
#include <droneid/variance.h>
#include <boost/test/unit_test.hpp>

#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/top_block.h>
#include <gnuradio/random.h>

#include <MatlabEngine.hpp>
#include <MatlabDataArray.hpp>

namespace gr {
    namespace droneid {

        std::vector<gr_complex> create_noise(const uint32_t sample_count) {
            std::vector<gr_complex> samples(sample_count);
            gr::random rng;
            for (auto & sample : samples) {
                sample = rng.rayleigh_complex();
            }

            return samples;
        }

        std::vector<float> calculate_true_variance(const std::vector<gr_complex> & samples, const uint32_t window_size) {
            using namespace matlab::engine;
            using namespace matlab::data;

            ArrayFactory factory;

            auto matlab = startMATLAB();

            auto samples_matlab = factory.createArray<gr_complex>(
                    ArrayDimensions({samples.size(), 1}), &samples[0], &samples[samples.size() - 1]);

            matlab->setVariable("samples", samples_matlab);
            matlab->setVariable("window_size", factory.createScalar(window_size));

            const auto program =
                    u"scores = zeros(length(samples) - window_size, 1);"
                    u"for idx = 1:length(scores)"
                    u"  window = samples(idx:idx + window_size - 1);"
                    u"  scores(idx) = var(window);"
                    u"end";

            matlab->eval(program);

            const auto scores = matlab->getVariable("scores");

            std::vector<float> output(scores.getNumberOfElements());
            for (uint32_t idx = 0; idx < output.size(); idx++) {
                output[idx] = static_cast<float>(scores[idx]);
            }

            return output;
        }

        BOOST_AUTO_TEST_CASE(foo2) {
            auto tb = gr::make_top_block("top");

            const auto sample_count = 12000;
            const auto window_size = 1024;

            const auto noise = create_noise(sample_count);

//            std::vector<gr_complex> kk(sample_count);
//            for (uint32_t idx = 0; idx < sample_count; idx++) {
//                kk[idx] = {(float)idx, (float)idx};
//            }

            auto source = gr::blocks::vector_source<gr_complex>::make(noise);
            auto sink = gr::blocks::vector_sink<float>::make();

            auto uut = gr::droneid::variance::make(window_size);

            tb->connect(source, 0, uut, 0);
            tb->connect(uut, 0, sink, 0);

            tb->run();

//            tb->start();
//
//            uint32_t last_sample_count = 0;
//            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//            while(true) {
//                if (last_sample_count != sink->data().size()) {
//                    last_sample_count = sink->data().size();
//                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                } else {
//                    break;
//                }
//            }
//
//            tb->stop();

            const auto expected_values = calculate_true_variance(noise, window_size);
            const auto & uut_output = sink->data();

            std::cout << "Got " << expected_values.size() << " elements from MATLAB and ";
            std::cout << uut_output.size() << " elements from GNU Radio\n";

            auto matlab = matlab::engine::startMATLAB();
            matlab::data::ArrayFactory factory;

            const auto matlab_values = factory.createArray<float>(
                    matlab::data::ArrayDimensions({expected_values.size(), 1}),
                    &expected_values[0], &expected_values[expected_values.size() - 1]);

            const auto uut_values = factory.createArray<float>(
                    matlab::data::ArrayDimensions({uut_output.size(), 1}),
                    &uut_output[0], &uut_output[uut_output.size() - 1]);

            matlab->setVariable("matlab", matlab_values);
            matlab->setVariable("uut", uut_values);
            matlab->setVariable("window_size", factory.createScalar(window_size));

            matlab->eval(
                    u"figure(1);"
                    u"subplot(3, 1, 1); plot(matlab); title('MATLAB');"
                    u"subplot(3, 1, 2); plot(uut); title('UUT');"
                    u"subplot(3, 1, 3); plot(uut - matlab); title('Delta');"
                    u"pause");

        }

    } /* namespace droneid */
} /* namespace gr */

