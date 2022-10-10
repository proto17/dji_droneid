/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DJI_DRONEID_UTILS_H
#define INCLUDED_DJI_DRONEID_UTILS_H

#include <gnuradio/dji_droneid/api.h>
#include <complex>
#include <cstdint>
#include <vector>
#include <memory>

namespace gr {
namespace dji_droneid {

/*!
 * \brief <+description+>
 *
 */
class DJI_DRONEID_API utils
{
public:
    utils() = default;
    ~utils() = default;

    // Hz between each carrier in DroneID
    static constexpr float CARRIER_SPACING = 15e3;

    // Number of OFDM data carriers in DroneID (independent of sample rate)
    static constexpr uint32_t DATA_CARRIER_COUNT = 600;

    /**
     * Get the correct FFT size based on the sample rate
     * @param sample_rate Sample rate (in Hz) of the collected samples
     * @return Number of FFT bins required
     */
    static uint32_t get_fft_size(float sample_rate);

    /**
     * Get the correct sample rate based on the FFT size
     * @param fft_size Number of FFT bins
     * @return Sample rate (in Hz)
     */
    static float get_sample_rate(uint32_t fft_size);

    /**
     * Get a list of FFT bins (all positive, assuming that DC is in the middle) that should be used as data carriers
     * @param sample_rate Sample rate (in Hz)
     * @return List of utils::DATA_CARRIER_COUNT values denoting position of all data carriers
     */
    static std::vector<uint32_t> get_data_carrier_indices(float sample_rate);

    /**
     * Create a Zadoff Chu sequence of `length` elements using the root index `root`
     * @param root Root index
     * @param length Number of elements to create
     * @return Vector of complex floats
     */
    static std::vector<std::complex<float>> zadoff_chu(uint32_t root, uint32_t length);

    /**
     * Create a time domain ZC sequence for the specified OFDM symbol index (4 or 6)
     *
     * Will be `fft_size` elements long
     *
     * <b>This only currently works for drones that have two ZC sequences present!!!</b>
     * @param fft_size Number of FFT bins the sequence should be generated for
     * @param symbol_idx Symbol index to generate the sequence for (must be 4 or 6)
     * @return Complex float vector of `fft_bins` elements representing the gold ZC sequence in the time domain
     */
    static std::vector<std::complex<float>> create_zc(uint32_t fft_size, uint8_t symbol_idx);

    /**
     * Get the long and short cyclic prefix lengths for the provided sample rate
     * @param sample_rate Sample rate (in Hz)
     * @return Pair with the first element being the long cyclic prefix length in samples, second being the short
     */
    static std::pair<uint16_t, uint16_t> get_cyclic_prefix_lengths(float sample_rate);

    /**
     * Calculate the mean of a complex vector
     * @param samples Pointer to complex vector
     * @param sample_count Number of samples in the complex vector
     * @return Mean of the complex vector
     */
    static std::complex<float> mean_fast(const std::complex<float> * samples, uint32_t sample_count);

    static float variance_no_mean(const std::complex<float> * samples, uint32_t sample_count);
    static float variance(const std::complex<float> * samples, uint32_t sample_count);
private:
};

} // namespace dji_droneid
} // namespace gr

#endif /* INCLUDED_DJI_DRONEID_UTILS_H */
