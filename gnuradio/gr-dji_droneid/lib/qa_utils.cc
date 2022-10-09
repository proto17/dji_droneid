/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/attributes.h>
#include <gnuradio/dji_droneid/utils.h>
#include <boost/test/unit_test.hpp>
#include <MatlabDataArray.hpp>
#include <MatlabEngine.hpp>

namespace gr {
namespace dji_droneid {

using namespace matlab::engine;

std::unique_ptr<MATLABEngine> matlab_engine = nullptr;
matlab::data::ArrayFactory factory;

struct MatlabHandler {
    MatlabHandler() {
        matlab_engine = startMATLAB();
        matlab_engine->eval(u"addpath(genpath('" MATLAB_SOURCE_PATH "'));");
    }

    ~MatlabHandler() {
        matlab_engine = nullptr;
    }
};

BOOST_GLOBAL_FIXTURE(MatlabHandler);

struct TestFixture {
    TestFixture () {
        BOOST_REQUIRE(matlab_engine != nullptr);
    }

    static void setup() {
        // Since the MATLAB instance is never reset, the workspace needs to be cleared each time for sanity
        TestFixture::clear_workspace();
    }

    ~TestFixture() = default;

    static void clear_workspace() {
        matlab_engine->eval(u"clear all;");
    }

    static uint32_t get_fft_size(const float sample_rate) {
        const auto ret = matlab_engine->feval("get_fft_size", factory.createScalar(sample_rate));
        BOOST_REQUIRE_EQUAL(ret.getNumberOfElements(), 1);
        return static_cast<uint32_t>(ret[0]);
    }

    static std::vector<uint32_t> get_data_carrier_indices(const float sample_rate) {
        const auto ret = matlab_engine->feval("get_data_carrier_indices", factory.createScalar(sample_rate));
        BOOST_REQUIRE_EQUAL(ret.getNumberOfElements(), utils::DATA_CARRIER_COUNT);

        std::vector<uint32_t> ret_converted(ret.getNumberOfElements());
        for (auto idx = decltype(ret.getNumberOfElements()){0}; idx < ret.getNumberOfElements(); idx++) {
            ret_converted[idx] = static_cast<uint32_t>(ret[idx]) - 1;
        }

        return ret_converted;
    }

    static std::vector<std::complex<float>> create_zc(const uint32_t fft_size, const uint8_t symbol_idx) {
        matlab_engine->setVariable("fft_size", factory.createScalar(static_cast<double>(fft_size)));
        matlab_engine->setVariable("symbol_idx", factory.createScalar(static_cast<double>(symbol_idx)));
        matlab_engine->eval(u"zc = create_zc(fft_size, symbol_idx);");

        const matlab::data::TypedArray<std::complex<double>> ret = matlab_engine->getVariable("zc");

        const auto num_elements = ret.getNumberOfElements();

        BOOST_REQUIRE_EQUAL(static_cast<int>(ret.getType()), static_cast<int>(matlab::data::ArrayType::COMPLEX_DOUBLE));
        BOOST_REQUIRE_EQUAL(num_elements, fft_size);

        std::vector<std::complex<float>> ret_converted(num_elements);
        for (auto idx = decltype(num_elements){0}; idx < num_elements; idx++) {
            const std::complex<double> & temp = ret[idx];
            ret_converted[idx] = {
                static_cast<float>(temp.real()),
                static_cast<float>(temp.imag())
            };
        }

        clear_workspace();

        return ret_converted;
    }

    static std::vector<std::complex<float>> zadoff_chu(const uint32_t root, const uint32_t length) {
        matlab_engine->setVariable("root", factory.createScalar(static_cast<double>(root)));
        matlab_engine->setVariable("len", factory.createScalar(static_cast<double>(length)));
        matlab_engine->eval(u"ret = single(zadoffChuSeq(root, len));");
        const matlab::data::TypedArray<std::complex<float>> ret = matlab_engine->getVariable("ret");
        return {ret.begin(), ret.end()};
    }

    static std::pair<uint16_t, uint16_t> get_cyclic_prefix_lengths(const float sample_rate) {
        matlab_engine->setVariable("sample_rate", factory.createScalar(static_cast<double>(sample_rate)));
        matlab_engine->eval(u"[long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);");
        const auto long_cp_len = matlab_engine->getVariable("long_cp_len");
        const auto short_cp_len = matlab_engine->getVariable("short_cp_len");
        BOOST_REQUIRE_EQUAL(long_cp_len.getNumberOfElements(), 1);
        BOOST_REQUIRE_EQUAL(short_cp_len.getNumberOfElements(), 1);

        return {
            static_cast<uint16_t>(long_cp_len[0]),
            static_cast<uint16_t>(short_cp_len[0])
        };
    }
};

BOOST_FIXTURE_TEST_SUITE(Utils_Test_Suite, TestFixture);

BOOST_AUTO_TEST_CASE(test_utils__get_fft_size)
{
    const std::vector<float> rates = {15.36e6, 30.72e6, 61.44e6};
    for (const auto & rate : rates) {
        const auto expected = get_fft_size(rate);
        const auto calculated = utils::get_fft_size(rate);

        BOOST_REQUIRE_EQUAL(expected, calculated);
    }

    BOOST_REQUIRE_THROW(utils::get_fft_size(10.1e6), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_utils__get_data_carrier_indices)
{
    const std::vector<float> rates = {15.36e6, 30.72e6, 61.44e6};
    for (const auto & rate : rates) {
        const auto expected = get_data_carrier_indices(rate);
        const auto calculated = utils::get_data_carrier_indices(rate);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(), calculated.begin(), calculated.end());
    }
}

BOOST_AUTO_TEST_CASE(test_utils__get_sample_rate) {
    const std::vector<uint32_t> fft_sizes = {1024, 2048, 4096, 8192, 16384};
    for (const auto & fft_size : fft_sizes) {
        const auto expected = static_cast<float>(fft_size) * utils::CARRIER_SPACING;
        const auto calculated = utils::get_sample_rate(fft_size);

        BOOST_REQUIRE_EQUAL(expected, calculated);
    }
}

BOOST_AUTO_TEST_CASE(test_utils__create_zc) {
    const std::vector<uint32_t> lengths = {1024, 2048, 4096, 8192};
    const std::vector<uint8_t> symbol_indices = {4, 6};

    for (const auto & length : lengths) {
        for (const auto & symbol_index : symbol_indices) {
            auto calculated = utils::create_zc(length, symbol_index);
            const auto expected = create_zc(length, symbol_index);

            BOOST_REQUIRE_EQUAL(expected.size(), calculated.size());

            const double resolution = std::numeric_limits<float>::epsilon();
            for (auto idx = decltype(calculated.size()){0}; idx < calculated.size(); idx++) {
                const auto & expected_val = expected[idx];
                const auto & calculated_val = calculated[idx];

                BOOST_REQUIRE_LT(std::abs(expected_val.real() - calculated_val.real()), resolution);
                BOOST_REQUIRE_LT(std::abs(expected_val.imag() - calculated_val.imag()), resolution);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_utils__zadoff_chu) {
    std::vector<uint32_t> roots = {600, 147};
    std::vector<uint32_t> lengths = {601};

    for (const auto & root : roots) {
        for (const auto & length : lengths) {
            const auto expected = zadoff_chu(root, length);
            const auto calculated = utils::zadoff_chu(root, length);

            BOOST_REQUIRE_EQUAL(expected.size(), calculated.size());
            BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(), calculated.begin(), calculated.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_utils__get_cyclic_prefix) {
    std::vector<float> sample_rates = {15.36e6, 30.72e6, 61.44e6};

    for (const auto & sample_rate : sample_rates) {
        const auto expected = get_cyclic_prefix_lengths(sample_rate);
        const auto calculated = utils::get_cyclic_prefix_lengths(sample_rate);

        BOOST_REQUIRE_EQUAL(expected.first, calculated.first);
        BOOST_REQUIRE_EQUAL(expected.second, calculated.second);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} /* namespace dji_droneid */
} /* namespace gr */
