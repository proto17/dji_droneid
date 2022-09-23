/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/droneid/misc_utils.h>
#include <gnuradio/io_signature.h>

#include <numeric>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>
#include <gnuradio/random.h>
#include <iostream>

#include <volk/volk.h>

namespace gr {
namespace droneid {

misc_utils::misc_utils() {
}

misc_utils::~misc_utils() {
}

uint32_t misc_utils::get_long_cp_len(double sample_rate) {
    return static_cast<uint32_t>(std::round(sample_rate / 192000));
}

uint32_t misc_utils::get_short_cp_len(double sample_rate) {
    return static_cast<uint32_t>(round(0.0000046875 * sample_rate));
}

uint32_t misc_utils::get_fft_size(double sample_rate) {
    return static_cast<uint32_t>(round(sample_rate / CARRIER_SPACING));
}

std::vector<std::complex<float>> misc_utils::create_zc_sequence(const double sample_rate, const uint32_t root) {
    const auto fft_size = get_fft_size(sample_rate);
    std::vector<std::complex<float>> sequence(fft_size, {0, 0});

    const uint32_t guard_carriers = fft_size - OCCUPIED_CARRIERS_EXC_DC;
    const auto left_guards = guard_carriers / 2;

    std::vector<std::complex<float>> g (OCCUPIED_CARRIERS_INC_DC);
    const auto I = std::complex<double>(0, 1);
    for (int idx = 0; idx < OCCUPIED_CARRIERS_INC_DC; idx++) {
        // Doing the arith below in double precision and then casting down to a float.  Using floats the whole
        // way will result in an output that's very far off from the MATLAB implementation.  The errors will accumulate
        // down the vector
        sequence[left_guards + idx] = std::exp(-I * (M_PIf64 * (double)root * (double)idx * (double)(idx + 1) / 601.0));
    }

    // Null out the DC carrier
    sequence[(fft_size / 2)] = 0;

    // Create an FFT object that is configured to run an inverse FFT
    gr::fft::fft_complex_rev ifft(static_cast<int>(fft_size), false);

    // FFT-shift the inputs (swap the left and right halves) and store in the IFFT input buffer
    std::copy(sequence.begin() + (fft_size/2), sequence.begin() + fft_size, ifft.get_inbuf());
    std::copy(sequence.begin(), sequence.begin() + (fft_size/2), ifft.get_inbuf() + (fft_size/2));

    // Run the IFFT
    ifft.execute();

    // Copy the IFFT'd samples out
    std::copy(ifft.get_outbuf(), ifft.get_outbuf() + fft_size, sequence.begin());

    // The samples need to be scaled by the FFT size to get the power back down
    std::for_each(sequence.begin(), sequence.end(), [fft_size](std::complex<float> & sample){sample /= static_cast<float>(fft_size);});

    return sequence;
}

std::vector<std::complex<float>> misc_utils::conj(const std::vector<std::complex<float>> &input) {
    auto vec = input;
    std::for_each(vec.begin(), vec.end(), [](std::complex<float> & sample){sample = std::conj(sample);});
    return vec;
}

void misc_utils::write_samples_vec(const std::string &path, const std::vector<std::complex<float>> &samples) {
    write_samples(path, &samples[0], samples.size());
}

void misc_utils::write_samples(const std::string &path, const std::complex<float> * const samples, const uint32_t element_count) {
    write(path, samples, sizeof(samples[0]), element_count);
}

std::vector<uint32_t> misc_utils::get_data_carrier_indices(const uint32_t fft_size) {
    std::vector<uint32_t> data_carrier_indices(600, 0);

    auto * ptr = &data_carrier_indices[0];
    for (uint32_t idx = (fft_size / 2) - 300; idx < (fft_size / 2) + 301; idx++) {
        if (idx != fft_size / 2) {
            *ptr++ = idx;
        }
    }

    return data_carrier_indices;
}

std::vector<std::complex<float>> misc_utils::extract_data_carriers(const std::vector<std::complex<float>> & symbol, const uint32_t fft_size) {
    const auto data_carrier_indices = get_data_carrier_indices(fft_size);
    std::vector<std::complex<float>> data_carriers(data_carrier_indices.size());

    auto iter = data_carriers.begin();
    for (const auto & index : data_carrier_indices) {
        *iter++ = symbol[index];
    }

    return data_carriers;
}

void
misc_utils::write(const std::string &path, const void * const elements, const uint32_t element_size, const uint32_t element_count) {
    FILE * handle = fopen(path.c_str(), "wb");
    if (!handle) {
        throw std::runtime_error("Failed to open output file");
    }
    fwrite(elements, element_size, element_count, handle);
    fclose(handle);
}

void misc_utils::write_vec(const std::string &path, const std::vector<uint32_t> &elements) {
    write(path, &elements[0], sizeof(elements[0]), elements.size());
}

void misc_utils::print_bits(const std::vector<int8_t> & bits) {
    std::ostringstream buff;
    for (const auto & bit : bits) {
        buff << (bit == 0 ? 0 : 1);
    }
    buff << "\n";
    std::cout << buff.str();
    std::flush(std::cout);
}

std::string misc_utils::bit_vec_to_string(const std::vector<int8_t> &bit_vec) {
    std::ostringstream buff;

    char lut[2] = {'0', '1'};
    for (const auto & bit : bit_vec) {
        buff << lut[bit];
    }

    return buff.str();
}

uint32_t
misc_utils::find_zc_seq_start_idx(const std::vector<std::complex<float>> & samples, const double sample_rate) {
    const auto fft_size = get_fft_size(sample_rate);

    std::vector<float> scores(samples.size() - fft_size);
    std::vector<std::complex<float>> window_one(fft_size / 2);
    std::vector<std::complex<float>> window_two(fft_size / 2);

    auto window_one_iter = samples.begin();
    auto window_two_iter = samples.begin() + (fft_size / 2);

    std::vector<std::complex<float>> dot_prods(scores.size(), 0);

    for (uint32_t idx = 0; idx < scores.size(); idx++) {
        std::copy(window_one_iter, window_one_iter + (fft_size / 2), window_one.begin());
        std::copy(window_two_iter, window_two_iter + (fft_size / 2), window_two.begin());

        window_one_iter++;
        window_two_iter++;

        std::reverse(window_two.begin(), window_two.end());

        for (int i = 0; i < fft_size / 2; i++) {
            dot_prods[idx] += window_one[i] * std::conj(window_two[i]);
        }
    }

    volk_32fc_magnitude_squared_32f(&scores[0], &dot_prods[0], dot_prods.size());

    gr::droneid::misc_utils::write_samples_vec("/tmp/dots", dot_prods);
    gr::droneid::misc_utils::write("/tmp/scores", &scores[0], sizeof(scores[0]), scores.size());

    uint32_t max_idx;
    volk_32f_index_max_32u(&max_idx, &scores[0], scores.size());

    return max_idx;
}

std::vector<std::complex<float>>
misc_utils::read_samples(const std::string &file_path, uint32_t offset, uint32_t total_samples) {
    const auto element_size = sizeof(std::complex<float>);
    std::vector<std::complex<float>> samples;

    FILE * file_handle = fopen(file_path.c_str(), "r");
    if (! file_handle) {
        throw std::runtime_error("File '" + file_path + "' could not be opened");
    }

    fseek(file_handle, 0, SEEK_END);
    const auto available_samples = static_cast<uint32_t>(static_cast<double>(ftell(file_handle)) / element_size);
    if (available_samples < offset) {
        throw std::runtime_error("Not enough samples available to be read");
    }

    uint32_t samples_to_read = std::min(available_samples - offset, total_samples);
    if (total_samples == 0) {
        samples_to_read = available_samples - offset;
    }

    fseek(file_handle, static_cast<int32_t>(offset * element_size), SEEK_SET);

    samples.resize(samples_to_read);
    fread(&samples[0], element_size, samples_to_read, file_handle);
    fclose(file_handle);

    return samples;
}

double misc_utils::radians_to_hz(const double radians, const double sample_rate) {
    return radians * sample_rate / (2 * M_PIf64);
}

double misc_utils::hz_to_radians(const double frequency, const double sample_rate) {
    return 2 * M_PIf64 * frequency / sample_rate;
}

std::vector<uint32_t> misc_utils::get_cyclic_prefix_schedule(double sample_rate) {
    const auto long_cp_len = get_long_cp_len(sample_rate);
    const auto short_cp_len = get_short_cp_len(sample_rate);

    return {
        long_cp_len,
        short_cp_len,
        short_cp_len,
        short_cp_len,
        short_cp_len,
        short_cp_len,
        short_cp_len,
        short_cp_len,
        long_cp_len
    };
}

std::vector<std::pair<std::vector<std::complex<float>>, std::vector<std::complex<float>>>>
misc_utils::extract_ofdm_symbol_samples(const std::vector<std::complex<float>> & samples, double sample_rate, uint32_t offset) {
    std::vector<std::pair<std::vector<std::complex<float>>, std::vector<std::complex<float>>>> outputs(9);
    const auto fft_size = get_fft_size(sample_rate);
    const auto cyclic_prefixes = get_cyclic_prefix_schedule(sample_rate);
    fft::fft_complex_fwd fft_engine(static_cast<int32_t>(fft_size), true);
    fft::fft_shift<std::complex<float>> fft_shifter(fft_size);

    auto samples_iter = samples.begin() + offset;
    for (uint32_t idx = 0; idx < outputs.size(); idx++) {
        samples_iter += cyclic_prefixes[idx];
        std::vector<std::complex<float>> time_domain(samples_iter, samples_iter + fft_size);
        std::vector<std::complex<float>> freq_domain(fft_size);
        std::copy(time_domain.begin(), time_domain.end(), fft_engine.get_inbuf());
        fft_engine.execute();
        std::copy(fft_engine.get_outbuf(), fft_engine.get_outbuf() + fft_size, freq_domain.begin());
        fft_shifter.shift(freq_domain);

        outputs[idx] = std::make_pair(time_domain, freq_domain);
        samples_iter += fft_size;
    }

    return outputs;
}

std::vector<std::complex<float>>
misc_utils::calculate_channel(const std::vector<std::complex<float>> &symbol, const double sample_rate,
                              const uint32_t symbol_idx) {
    assert(symbol.size() == 600);
    std::vector<std::complex<float>> channel(symbol.size(), {0, 0});
    const auto fft_size = get_fft_size(sample_rate);
    fft::fft_complex_fwd fft_engine(static_cast<int32_t>(fft_size), true);
    fft::fft_shift<std::complex<float>> fft_shifter(fft_size);

    uint32_t root;
    if (symbol_idx == 4) {
        root = 600;
    } else {
        root = 147;
    }

    auto zc_gold_seq = create_zc_sequence(sample_rate, root);
    write_samples_vec("/tmp/zc_time_" + std::to_string(symbol_idx), zc_gold_seq);
    std::copy(zc_gold_seq.begin(), zc_gold_seq.end(), fft_engine.get_inbuf());
    fft_engine.execute();
    std::copy(fft_engine.get_outbuf(), fft_engine.get_outbuf() + fft_size, zc_gold_seq.begin());
    fft_shifter.shift(zc_gold_seq);
    write_samples_vec("/tmp/zc_freq_" + std::to_string(symbol_idx), zc_gold_seq);

    const auto zc_data_only = extract_data_carriers(zc_gold_seq, fft_size);
    write_samples_vec("/tmp/zc_data_" + std::to_string(symbol_idx), zc_data_only);

    for (uint32_t idx = 0; idx < symbol.size(); idx++) {
        channel[idx] = zc_data_only[idx] / symbol[idx];
    }

    return channel;
}

std::vector<int8_t> misc_utils::qpsk_to_bits(const std::vector<std::complex<float>> &samples) {
    std::vector<int8_t> bits(samples.size() * 2);

    auto * bit_vec_ptr = &bits[0];

    std::for_each(samples.begin(), samples.end(), [&bit_vec_ptr](const gr_complex & sample) {
        if (sample.real() > 0 && sample.imag() > 0) {
            *bit_vec_ptr++ = 0;
            *bit_vec_ptr++ = 0;
        } else if (sample.real() > 0 && sample.imag() < 0) {
            *bit_vec_ptr++ = 0;
            *bit_vec_ptr++ = 1;
        } else if (sample.real() < 0 && sample.imag() > 0) {
            *bit_vec_ptr++ = 1;
            *bit_vec_ptr++ = 0;
        } else {
            *bit_vec_ptr++ = 1;
            *bit_vec_ptr++ = 1;
        }
    });

    return bits;
}


std::complex<float> misc_utils::mean(const std::complex<float> *const samples, const uint32_t sample_count) {
    auto sum = std::accumulate(samples, samples + sample_count, std::complex<float>{0, 0});
    return sum / static_cast<float>(sample_count);
}

float misc_utils::var(const std::complex<float> *const samples, uint32_t sample_count) {
    const auto mean_val = mean(samples, sample_count);
    const auto recip = 1.0f / static_cast<float>(sample_count - 1);
    float var_val = 0;

    for (uint32_t idx = 0; idx < sample_count; idx++) {
        const auto sample = samples[idx] - mean_val;
        var_val += (static_cast<float>(std::pow(sample.real(), 2) + std::pow(sample.imag(), 2))) * recip;
    }

    return var_val;
}

float misc_utils::var_no_mean(const std::complex<float> *samples, uint32_t sample_count) {
    const auto recip = 1.0f / static_cast<float>(sample_count - 1);
    float var_val = 0;

    for (uint32_t idx = 0; idx < sample_count; idx++) {
        const auto & sample = samples[idx];
        var_val += (static_cast<float>(std::pow(sample.real(), 2) + std::pow(sample.imag(), 2))) * recip;
    }

    return var_val;
}

std::vector<float> misc_utils::abs_squared(const std::vector<std::complex<float>> &samples) {
    std::vector<float> ret(samples.size());

    for (auto idx = decltype(samples.size()){0}; idx < samples.size(); idx++) {
        const auto & sample = samples[idx];
        ret[idx] = (sample.real() * sample.real()) + (sample.imag() * sample.imag());
    }

    return ret;
}

std::vector<std::complex<float>> misc_utils::create_gaussian_noise(const uint32_t sample_count) {
    std::vector<std::complex<float>> output(sample_count);
    gr::random rand;
    for (auto & sample : output) {
        sample = rand.rayleigh_complex();
    }

    return output;
}


} /* namespace droneid */
} /* namespace gr */