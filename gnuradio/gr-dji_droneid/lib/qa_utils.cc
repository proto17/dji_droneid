/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/attributes.h>
#include <gnuradio/dji_droneid/utils.h>
#include <gnuradio/filter/firdes.h>
#include <MatlabDataArray.hpp>
#include <MatlabEngine.hpp>
#include <boost/test/unit_test.hpp>
#include <thread>

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

    static void setVariable(const std::string & name, const std::vector<std::complex<float>> & samples) {
        matlab_engine->setVariable(name, factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
    }

    static void setVariable(const std::string & name, const double val) {
        matlab_engine->setVariable(name, factory.createScalar(val));
    }

    static std::vector<std::complex<float>> getComplexVec(const std::string & name) {
        const matlab::data::TypedArray<std::complex<float>> ret = matlab_engine->getVariable(name);
        return {ret.begin(), ret.end()};
    }

    static double getScalar(const std::string & name) {
        return matlab_engine->getVariable(name)[0];
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

    static std::complex<float> mean(const std::vector<std::complex<float>> & samples) {
        matlab_engine->setVariable("samples", factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
        matlab_engine->eval(u"m = mean(samples);");
        const matlab::data::TypedArray<std::complex<float>> mean_val = matlab_engine->getVariable("m");

        BOOST_REQUIRE_EQUAL(mean_val.getNumberOfElements(), 1);

        matlab_engine->eval(u"clear samples m");

        return mean_val[0];
    }

    static float var(const std::vector<std::complex<float>> & samples) {
        matlab_engine->setVariable("samples", factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
        matlab_engine->eval(u"output = single(var(samples));");
        const matlab::data::TypedArray<float> output = matlab_engine->getVariable("output");
        matlab_engine->eval(u"clear output samples");
        return output[0];
    }

    static std::vector<std::complex<float>> remove_mean(const std::vector<std::complex<float>> & samples) {
        matlab_engine->setVariable("samples", factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
        matlab_engine->eval(u"samples = single(samples - mean(samples));");
        const matlab::data::TypedArray<std::complex<float>> output_samples = matlab_engine->getVariable("samples");
        matlab_engine->eval(u"clear samples");

        BOOST_REQUIRE_EQUAL(output_samples.getNumberOfElements(), samples.size());

        return {output_samples.begin(), output_samples.end()};
    }

    static std::vector<std::complex<float>> conj(const std::vector<std::complex<float>> & samples) {
        matlab_engine->setVariable("samples", factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
        matlab_engine->eval(u"samples = single(conj(samples));");
        const matlab::data::TypedArray<std::complex<float>> output_samples = matlab_engine->getVariable("samples");
        matlab_engine->eval(u"clear samples");

        BOOST_REQUIRE_EQUAL(output_samples.getNumberOfElements(), samples.size());

        return {output_samples.begin(), output_samples.end()};
    }

    static std::vector<std::complex<float>> create_test_vector(const uint32_t sample_count) {
        matlab_engine->setVariable("sample_count", factory.createScalar(static_cast<double>(sample_count)));
        matlab_engine->eval(u"samples = single(complex(randn(1, sample_count), randn(1, sample_count)));");
        const matlab::data::TypedArray<std::complex<float>> samples_array = matlab_engine->getVariable("samples");
        std::vector<std::complex<float>> samples(samples_array.begin(), samples_array.end());
        matlab_engine->eval(u"clear samples");

        return samples;
    }

    static std::vector<std::complex<float>> interpolate(const std::vector<std::complex<float>> & samples, const uint32_t rate) {
        matlab_engine->setVariable("rate", factory.createScalar(static_cast<double>(rate)));
        matlab_engine->setVariable("samples", factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
        matlab_engine->eval(
            u""
            "interp_samples = zeros(1, length(samples) * rate);\n"
            "interp_samples(1:rate:end) = samples;\n"
            "interp_samples = single(interp_samples);");
        const matlab::data::TypedArray<std::complex<float>> interped_samples = matlab_engine->getVariable("interp_samples");
        matlab_engine->eval(u"clear interp_samples samples rate");
        return {interped_samples.begin(), interped_samples.end()};
    }

    static std::vector<std::complex<float>> filter(const std::vector<std::complex<float>> & samples, const std::vector<float> & taps) {
        matlab_engine->setVariable("taps", factory.createArray({1, taps.size()}, taps.begin(), taps.end()));
        matlab_engine->setVariable("samples", factory.createArray({1, samples.size()}, samples.begin(), samples.end()));
        matlab_engine->eval(
            u""
            "filtered = single(filter(taps, 1, samples));\n"
            "filtered = single(filtered(1:length(filtered) - length(taps)));"
        );
        const matlab::data::TypedArray<std::complex<float>> filtered_samples = matlab_engine->getVariable("filtered");
        matlab_engine->eval(u"clear filtered taps samples");
        return {filtered_samples.begin(), filtered_samples.end()};
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

BOOST_AUTO_TEST_CASE(test_utils__mean_fast) {
    const uint32_t iters = 100;
    const uint32_t length = 10000;

    matlab_engine->setVariable("sample_count", factory.createScalar(static_cast<double>(length)));
    for (uint32_t iter = 0; iter < iters; iter++) {
        matlab_engine->eval(u"samples = single(complex(randn(1, sample_count), randn(1, sample_count)));");
        const matlab::data::TypedArray<std::complex<float>> samples_array = matlab_engine->getVariable("samples");
        const std::vector<std::complex<float>> samples(samples_array.begin(), samples_array.end());

        const auto expected = mean(samples);
        const auto calculated = utils::mean_fast(&samples[0], samples.size());

        // MATLAB does all of its arithmetic in double precision, and the mean_fast function does everything in single
        // This means there is going to be a large delta between the values simply due to rounding errors.  The value
        // below is a percentage (eg 0.2 == a 0.2% delta) that the difference can be before failing the test
        const auto accuracy = 0.2;

        BOOST_REQUIRE_CLOSE(expected.real(), calculated.real(), accuracy);
        BOOST_REQUIRE_CLOSE(expected.imag(), calculated.imag(), accuracy);
    }
}

BOOST_AUTO_TEST_CASE(test_utils__variance) {
    const uint32_t iters = 100;
    const uint32_t length = 10000;

    for (auto iter = decltype(iters){0}; iter < iters; iter++) {
        const auto samples = create_test_vector(length);

        const auto expected = var(samples);
        const auto calculated = utils::variance(&samples[0], samples.size());

        BOOST_REQUIRE_CLOSE(expected, calculated, 0.002);
    }
}

BOOST_AUTO_TEST_CASE(test_utils__variance_vector) {
    const uint32_t iters = 100;
    const uint32_t length = 10000;

    for (auto iter = decltype(iters){0}; iter < iters; iter++) {
        const auto samples = create_test_vector(length);

        const auto expected = var(samples);
        const auto calculated = utils::variance_vector(samples);

        BOOST_REQUIRE_CLOSE(expected, calculated, 0.002);
    }
}

BOOST_AUTO_TEST_CASE(test_utils__variance_no_mean) {
    const uint32_t iters = 100;
    const uint32_t length = 10000;

    for (auto iter = decltype(iters){0}; iter < iters; iter++) {
        const auto samples = remove_mean(create_test_vector(length));

        const auto expected = var(samples);
        const auto calculated = utils::variance_no_mean(&samples[0], samples.size());

        BOOST_REQUIRE_CLOSE(expected, calculated, 0.002);
    }
}

BOOST_AUTO_TEST_CASE(test_utils__conj_vector) {
    const uint32_t iters = 100;
    const uint32_t length = 10000;

    for (auto iter = decltype(iters){0}; iter < iters; iter++) {
        const auto samples = create_test_vector(length);

        const auto expected = conj(samples);
        const auto calculated = utils::conj_vector(samples);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(), calculated.begin(), calculated.end());
    }
}

BOOST_AUTO_TEST_CASE(test_utils__interpolate) {
    const uint32_t sample_count = 10000;
    const std::vector<uint32_t> rates = {1, 2, 3, 4, 5, 6, 10};

    for (const auto & rate : rates) {
        const auto samples = create_test_vector(sample_count);

        const auto expected = interpolate(samples, rate);
        const auto calculated = utils::interpolate(samples, rate);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(), calculated.begin(), calculated.end());
    }
}

BOOST_AUTO_TEST_CASE(test_utils__filter) {
    const uint32_t sample_count = 10000;
    const auto taps = gr::filter::firdes::low_pass(1, 1, 0.4, 0.01);
    const auto samples = create_test_vector(sample_count);

    const auto expected = filter(samples, taps);
    const auto calculated = utils::filter(samples, taps);

//    {
//        setVariable("expected", expected);
//        setVariable("calculated", calculated);
//
//        matlab_engine->eval(
//            u""
//            "length(expected)\n"
//            "length(calculated)\n"
//            "expected(1)\n"
//            "calculated(1)\n"
//            "figure(1); \n"
//            "subplot(3, 1, 1); plot(real(expected));\n"
//            "subplot(3, 1, 2); plot(real(calculated));\n"
//            "subplot(3, 1, 3); plot(real(calculated) - real(expected));"
//            "figure(2); \n"
//            "subplot(3, 1, 1); plot(imag(expected));\n"
//            "subplot(3, 1, 2); plot(imag(calculated));\n"
//            "subplot(3, 1, 3); plot(imag(calculated) - imag(expected));"
//        );
//
//        std::this_thread::sleep_for(std::chrono::seconds(5));
//    }

    BOOST_REQUIRE_EQUAL(expected.size(), calculated.size());

    const auto max_delta = 0.00001f;
//    for (auto idx = decltype(expected.size()){0}; idx < expected.size(); idx++) {
//        BOOST_REQUIRE_LE(std::abs(expected[idx].real() - calculated[idx].real()), max_delta);
//        BOOST_REQUIRE_LE(std::abs(expected[idx].imag() - calculated[idx].imag()), max_delta);
//    }
}

BOOST_AUTO_TEST_SUITE_END()

} /* namespace dji_droneid */
} /* namespace gr */
