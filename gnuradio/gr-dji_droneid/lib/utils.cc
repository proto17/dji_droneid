/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/dji_droneid/utils.h>
#include <gnuradio/io_signature.h>

#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>
#include <gnuradio/filter/fft_filter.h>
#include <fftw3.h>
#include <volk/volk.h>

namespace gr {
namespace dji_droneid {

std::vector<uint32_t> utils::get_data_carrier_indices(const float sample_rate)
{
    const auto fft_size = get_fft_size(sample_rate);
    const auto dc = ((fft_size / 2) + 1) - 1;
    std::vector<uint32_t> mapping(DATA_CARRIER_COUNT, 0);

    auto mapping_ptr = &mapping[0];

    for (uint32_t idx = dc - (DATA_CARRIER_COUNT / 2); idx <= dc - 1; idx++) {
        *mapping_ptr++ = idx;
    }

    for (uint32_t idx = dc + 1; idx <= dc + (DATA_CARRIER_COUNT / 2); idx++) {
        *mapping_ptr++ = idx;
    }

    return mapping;
}

uint32_t utils::get_fft_size(const float sample_rate) {
    const auto fft_size = sample_rate / CARRIER_SPACING;
    if (fft_size != std::round(fft_size)) {
        throw std::runtime_error("Invalid sample rate.  Did not produce integer number of FFT carriers");
    }

    return static_cast<uint32_t>(fft_size);
}

std::vector<std::complex<float>> utils::create_zc(const uint32_t fft_size, const uint8_t symbol_idx)
{
    if (symbol_idx != 4 && symbol_idx != 6) {
        throw std::runtime_error("Symbol idx must be 4 or 6");
    }

    if (std::log2(fft_size) != std::round(std::log2(fft_size))) {
        throw std::runtime_error("FFT size must be an even power of 2");
    }

    const auto root = (symbol_idx == 4) ? 600 : 147;

    auto samples = zadoff_chu(root, DATA_CARRIER_COUNT + 1);

    // Remove the center ZC value
    samples.erase(samples.begin() + (DATA_CARRIER_COUNT / 2));

    const auto data_carrier_indices = get_data_carrier_indices(get_sample_rate(fft_size));

    // Important that the elements of the input buffer be zeroed since not all of the elements will be populated
    std::vector<std::complex<double>> fft_input(fft_size, 0);
    std::vector<std::complex<double>> fft_output(fft_size);

    for (auto idx = decltype(data_carrier_indices.size()){0}; idx < data_carrier_indices.size(); idx++) {
        fft_input[data_carrier_indices[idx]] = samples[idx];
    }

    gr::fft::fft_shift<std::complex<double >> shifter(fft_size);
    shifter.shift(fft_input);

    // GNU Radio's FFT object can't do complex double, so using FFTW directly
    auto plan = fftw_plan_dft_1d(static_cast<int>(fft_size),
                                  reinterpret_cast<fftw_complex *>(&fft_input[0]),
                                  reinterpret_cast<fftw_complex *>(&fft_output[0]),
                                  FFTW_BACKWARD, FFTW_ESTIMATE);

    fftw_execute(plan);

    // Normalize the output of the FFT by dividing each sample by the number of FFT bins
    std::for_each(fft_output.begin(), fft_output.end(), [&fft_size](std::complex<double> & sample){
        sample = {
            sample.real() / static_cast<double>(fft_size),
            sample.imag() / static_cast<double>(fft_size)
        };
    });

    // Convert the complex doubles to complex floats (single).  This annoyingly cannot be done as a single operation :(
    std::vector<std::complex<float>> output(fft_size);
    for (auto idx = decltype(fft_output.size()){0}; idx < fft_output.size(); idx++) {
        output[idx] = {
            static_cast<float>(fft_output[idx].real()),
            static_cast<float>(fft_output[idx].imag())
        };
    }

    fftw_destroy_plan(plan);

    return output;
}
float utils::get_sample_rate(const uint32_t fft_size) {
    return static_cast<float>(fft_size) * CARRIER_SPACING;
}

std::vector<std::complex<float>> utils::zadoff_chu(const uint32_t root, const uint32_t length)
{
    std::vector<std::complex<float>> ret(length);
    const auto root_double = static_cast<double>(root);
    const auto length_double = static_cast<double>(length);
    for (auto idx = decltype(length){0}; idx < length; idx++) {
        const auto idx_double = static_cast<double>(idx);
        const double temp = M_PIf64 * root_double * idx_double * (idx_double + 1) / length_double;
        ret[idx] = {
            static_cast<float>(std::cos(temp)),
            static_cast<float>(-std::sin(temp))
        };
    }

    return ret;
}
std::pair<uint16_t, uint16_t> utils::get_cyclic_prefix_lengths(const float sample_rate)
{
    return std::make_pair(
        static_cast<uint16_t>((1.0 / 192e3) * sample_rate),
        static_cast<uint16_t>((0.0000046875 * sample_rate))
    );
}

std::complex<float> utils::mean_fast(const std::complex<float> * const samples, const uint32_t sample_count)
{
    // Treat the vector of complex floats as a single vector of float values
    auto sample_floats = reinterpret_cast<const float *>(samples);
    float real = 0, imag = 0;

    for (uint32_t idx = 0; idx < sample_count; idx++) {
        real += *sample_floats++;
        imag += *sample_floats++;
    }

    real = real / static_cast<float>(sample_count);
    imag = imag / static_cast<float>(sample_count);

    return {real, imag};
}

float utils::variance_no_mean(const std::complex<float> * const samples, const uint32_t sample_count)
{
    float total = 0;

    for (auto idx = decltype(sample_count){0}; idx < sample_count; idx++) {
        const auto & sample = samples[idx];
        total += (sample.real() * sample.real()) + (sample.imag() * sample.imag());
    }

    return total / static_cast<float>(sample_count - 1);
}

float utils::variance(const std::complex<float> * const samples, const uint32_t sample_count)
{
    float total = 0;
    const auto mean = mean_fast(samples, sample_count);

    for (auto idx = decltype(sample_count){0}; idx < sample_count; idx++) {
        auto sample = samples[idx] - mean;
        total += (sample.real() * sample.real()) + (sample.imag() * sample.imag());
    }

    return total / static_cast<float>(sample_count - 1);
}

std::vector<std::complex<float>>
utils::conj_vector(const std::vector<std::complex<float>>& samples)
{
    std::vector<std::complex<float>> output(samples.size());
    for (auto idx = decltype(samples.size()){0}; idx < samples.size(); idx++) {
        const auto & sample = samples[idx];
        output[idx] = {
            sample.real(),
            -sample.imag()
        };
    }

    return output;
}

float utils::variance_vector(const std::vector<std::complex<float>>& samples)
{
    return variance(&samples[0], samples.size());
}

uint32_t utils::xcorr_in_place(const std::complex<float> * const samples,
                           const std::complex<float> * const pattern,
                           std::complex<float> * const output,
                           const uint32_t sample_count,
                           const uint32_t pattern_sample_count,
                           const bool needs_conj)
{
    if (sample_count < pattern_sample_count) {
        throw std::runtime_error("Pattern count cannot be greater than sample count");
    }

    if (sample_count == pattern_sample_count) {
        throw std::runtime_error("Equal pattern count and sample count is not supported at this time");
    }

    const auto total_steps = sample_count - pattern_sample_count;

    if (needs_conj) {
        for (auto idx = decltype(total_steps){ 0 }; idx < total_steps; idx++) {
            volk_32fc_x2_conjugate_dot_prod_32fc(
                output + idx, samples + idx, pattern, pattern_sample_count);
        }
    } else {
        for (auto idx = decltype(total_steps){ 0 }; idx < total_steps; idx++) {
            volk_32fc_x2_dot_prod_32fc(
                output + idx, samples + idx, pattern, pattern_sample_count);
        }
    }

    return total_steps;
}

std::vector<std::complex<float>>
utils::xcorr_vector(const std::vector<std::complex<float>>& samples,
                    const std::vector<std::complex<float>>& pattern, const bool needs_conj)
{
    if (samples.size() < pattern.size()) {
        throw std::runtime_error("Pattern count cannot be greater than sample count");
    }

    if (samples.size() == pattern.size()) {
        throw std::runtime_error("Equal pattern count and sample count is not supported at this time");
    }

    std::vector<std::complex<float>> output(samples.size() - pattern.size());

    xcorr_in_place(&samples[0], &pattern[0], &output[0], samples.size(), pattern.size(), needs_conj);

    return output;
}

void utils::mag_squared(const std::complex<float> * const samples,
                        float * const output,
                        const uint32_t sample_count)
{
    volk_32fc_magnitude_squared_32f(output, samples, sample_count);
}

void utils::mag_squared_vector_in_place(const std::vector<std::complex<float>>& samples,
                                        std::vector<float>& output)
{
    mag_squared(&samples[0], &output[0], samples.size());
}

std::vector<float>
utils::mag_squared_vector(const std::vector<std::complex<float>>& samples)
{
    std::vector<float> output(samples.size());
    mag_squared_vector_in_place(samples, output);
    return output;
}

uint32_t utils::get_burst_sample_count(const float sample_rate) {
    const auto fft_size = get_fft_size(sample_rate);
    const auto [long_cp_len, short_cp_len] = get_cyclic_prefix_lengths(sample_rate);

    return (fft_size * 9) + long_cp_len + (short_cp_len * 8);
}

std::vector<float> utils::mag_vector(const std::vector<std::complex<float>>& samples)
{
    std::vector<float> output(samples.size());
    volk_32fc_magnitude_32f(&output[0], &samples[0], samples.size());
    return output;
}

void utils::write_samples(const std::string& path,
                          const std::complex<float>* const samples,
                          const uint32_t sample_count)
{
    FILE * fh = fopen(path.c_str(), "w");
    if (! fh) {
        throw std::runtime_error("Could not open output file '" + path + "'");
    }

    fwrite(samples, sizeof(samples[0]), sample_count, fh);
    fclose(fh);
}

void utils::write_samples_vector(const std::string& path,
                                 const std::vector<std::complex<float>>& samples)
{
    write_samples(path, &samples[0], samples.size());
}

std::vector<std::complex<float>>
utils::interpolate(const std::vector<std::complex<float>>& samples, uint32_t rate)
{
    std::vector<std::complex<float>> ret(samples.size() * rate, 0);
    auto * ret_ptr = &ret[0];
    for (auto idx = decltype(samples.size()){0}; idx < samples.size(); idx++) {
        *ret_ptr = samples[idx];
        ret_ptr += rate;
    }
    return ret;
}

std::vector<std::complex<float>>
utils::filter(const std::vector<std::complex<float>>& samples,
              const std::vector<float>& taps)
{
    gr::filter::kernel::fft_filter_ccf filter(1, taps);
    std::vector<std::complex<float>> output(samples.size() + (taps.size() * 2));
    // TODO(16Oct2022): This might be an invalid read.  May need to be samples.size() - taps.size()?
    const auto count = filter.filter(static_cast<int32_t>(samples.size()), &samples[0], &output[0]);

    // Only the first `count` samples are actually valid.  Trim off the remaining samples
    output.erase(output.begin() + static_cast<int32_t>(count - taps.size()), output.end());
    return output;
}

} /* namespace dji_droneid */
} /* namespace gr */
