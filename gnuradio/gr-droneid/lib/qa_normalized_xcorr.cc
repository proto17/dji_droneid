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
#include <type_traits>

#include <droneid/misc_utils.h>

#include <MatlabEngine.hpp>
#include <MatlabDataArray.hpp>

using namespace matlab::engine;

namespace gr {
    namespace droneid {
        using sample_t = float;
        using complex_t = std::complex<sample_t>;
        using complex_vec_t = std::vector<complex_t>;

        template <typename OUTPUT_T>
        std::vector<OUTPUT_T> run(const std::u16string & cmd, std::vector<matlab::data::Array> & inputs, MATLABEngine * engine) {
            std::vector<OUTPUT_T> output;

            matlab::data::Array matlab_output = engine->feval(cmd, inputs);

            output.resize(matlab_output.getNumberOfElements());
            for (uint32_t idx = 0; idx < matlab_output.getNumberOfElements(); idx++) {
                output[idx] = static_cast<OUTPUT_T>(matlab_output[idx]);
            }

            return output;
        }

        template <typename OUTPUT_T>
        std::vector<OUTPUT_T> get_vec(const std::u16string & variable_name, MATLABEngine * const engine_ptr) {
            std::vector<OUTPUT_T> output;

            auto variable_value = engine_ptr->getVariable(variable_name);
            output.reserve(variable_value.getNumberOfElements());

            for (const auto & sample : matlab::data::getReadOnlyElements<OUTPUT_T>(variable_value)) {
                output.push_back(sample);
            }

            return output;
        }

        template <typename OUTPUT_T>
        std::vector<std::complex<OUTPUT_T>> get_complex_vec(const std::u16string & variable_name, MATLABEngine * const engine_ptr) {
            std::vector<std::complex<OUTPUT_T>> output;

            auto variable_value = engine_ptr->getVariable(variable_name);
            output.reserve(variable_value.getNumberOfElements());
            for (const std::complex<double> & sample : matlab::data::getReadOnlyElements<std::complex<double>>(variable_value)) {
                output.push_back({
                         static_cast<OUTPUT_T>(sample.real()), static_cast<OUTPUT_T>(sample.imag())
                 });
            }

            return output;
        }

        uint64_t get_matrix_length(const std::u16string & variable_name, MATLABEngine * const engine_ptr) {
            const auto cmd = u"length(" + variable_name + u");";
            engine_ptr->eval(cmd);
            const auto ret = engine_ptr->getVariable("ans");
            return static_cast<uint64_t>(ret[0]);
        }

        BOOST_AUTO_TEST_CASE(foo) {
            auto matlab_ptr = startMATLAB();
            matlab::data::ArrayFactory factory;


            gr::random random;
            const auto burst = misc_utils::read_samples("/tmp/droneid_debug/burst_25", 0, 0);

            const auto full_sample_count = static_cast<uint32_t>(.1e6);

            complex_vec_t samples(full_sample_count);
            const int64_t padding = floor((static_cast<int64_t>(full_sample_count) - static_cast<int64_t>(burst.size())) / 2);
            std::cout << "Padding: " << padding << "\n";
            if (padding > 0) {
                for (uint64_t idx = 0; idx < padding; idx++) {
                    samples[idx] = random.rayleigh_complex();
                }
                std::copy(burst.begin(), burst.end(), samples.begin() + padding);
                for (uint64_t idx = padding + burst.size(); idx < samples.size(); idx++) {
                    samples[idx] = random.rayleigh_complex();
                }
            } else {
                samples = burst;
            }

            auto filter_taps = misc_utils::create_zc_sequence(15.36e6, 600);
            filter_taps.resize(filter_taps.size() / 1);
            const auto filter_length = filter_taps.size();


            normalized_xcorr xcorr(filter_taps);
            std::vector<sample_t> mags(samples.size() - filter_length, 0);
            xcorr.run(&samples[0], samples.size(), &mags[0]);
            const int iters = 10;
            const auto start = std::chrono::high_resolution_clock::now();
            for (int iter = 0; iter < iters; iter++) {
                xcorr.run(&samples[0], samples.size(), &mags[0]);
            }
            const auto end = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration<float>(end - start).count();
            const auto rate = (samples.size() * iters) / duration;
            std::cout << "Took " << duration << " seconds to run through " << samples.size() << " samples\n";
            std::cout << "Average of " << rate << " samples per second\n";

            const auto max_element_iter = std::max_element(mags.begin(), mags.end());
            const auto distance = std::distance(mags.begin(), max_element_iter);
            std::cout << "c++ Distance: " << distance << "\n";


            complex_vec_t matlab_golden_mags;
            {
                auto samples_vec = factory.createArray<complex_t>(
                        matlab::data::ArrayDimensions({samples.size(), 1}),
                        &samples[0],
                        &samples[samples.size() - 1]);

                auto filter_taps_vec = factory.createArray<complex_t>(
                        matlab::data::ArrayDimensions({filter_taps.size(), 1}),
                        &filter_taps[0],
                        &filter_taps[filter_taps.size() - 1]);

                matlab_ptr->setVariable("samples", samples_vec);
                matlab_ptr->setVariable("filter", filter_taps_vec);

                const std::u16string correlation_script =
                        u"scores1 = zeros(length(samples) - length(filter), 1);\n"
                        u"scores2 = zeros(length(samples) - length(filter), 1);\n"
                        u"for idx = 1:length(scores1)\n"
                        u"  window = samples(idx:idx + length(filter) - 1);"
                        u"  scores1(idx) = xcorr(window, filter, 'normalized', 0);"
                        u"  scores2(idx) = sum(window .* conj(filter)) / length(filter);"
                        u"end";

                matlab_ptr->eval(correlation_script);


                matlab_golden_mags = get_complex_vec<sample_t>(u"scores1", matlab_ptr.get());
                std::cout << "Got back " << matlab_golden_mags.size() << " correlation scores\n";
            }

            const auto matlab_golden_mags_sqrd = misc_utils::abs_squared(matlab_golden_mags);

            matlab_ptr->setVariable(u"cpp_mags", factory.createArray<sample_t>(
                    matlab::data::ArrayDimensions({mags.size(), 1}), &mags[0], &mags[mags.size() - 1]));

            matlab_ptr->eval(u"delta = cpp_mags - (abs(scores1).^2);");

            const auto deltas = get_vec<sample_t>(u"delta", matlab_ptr.get());

            matlab_ptr->eval(u"figure(1); plot(delta);");
            matlab_ptr->eval(u"figure(2); subplot(3, 1, 1); plot(abs(cpp_mags)); title('CPP mags');");
            matlab_ptr->eval(u"figure(2); subplot(3, 1, 2); plot(abs(scores1).^2); title('MATLAB mags');");
            matlab_ptr->eval(u"figure(2); subplot(3, 1, 3); plot(10 * log10(abs(samples).^2)); title('Raw Samples');");
            matlab_ptr->eval(u"pause;");

        }
    } /* namespace droneid */
} /* namespace gr */

