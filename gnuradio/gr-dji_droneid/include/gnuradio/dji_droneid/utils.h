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

    /**
     * Calculate the variance of a complex vector where the mean is already zero
     * @param samples Pointer to complex vector
     * @param sample_count Number of samples in the complex vector
     * @return Variance of the complex vector
     */
    static float variance_no_mean(const std::complex<float> * samples, uint32_t sample_count);

    /**
     * Calculate the variance of a complex vector where the mean is not known to be zero
     *
     * This function is mostly the same as utils::variance_no_mean but does have to calculate the
     * mean value of the input vector and subtract it from each sample, so this will take longer
     * @param samples Pointer to complex vector
     * @param sample_count Number of samples in the complex vector
     * @return Variance of the complex vector
     */
    static float variance(const std::complex<float> * samples, uint32_t sample_count);

    /**
     * Calculate the variance of a complex vector where the mean is not known to be zero
     *
     * This function is mostly the same as utils::variance_no_mean but does have to calculate the
     * mean value of the input vector and subtract it from each sample, so this will take longer
     * @param samples Vector of complex samples
     * @return Variance of the complex vector
     */
    static float variance_vector(const std::vector<std::complex<float>> & samples);

    /**
     * Get the conjugate of the provided vector
     * @param samples Vector of complex samples
     * @return Conjugate of the input sample vector
     */
    static std::vector<std::complex<float>> conj_vector(const std::vector<std::complex<float>> & samples);

    /**
     * Run a cross correlation with the output vector provided
     * @param samples Vector of complex samples that should be searched through
     * @param pattern Pattern to search for
     * @param output Cross correlation scores for each possible shift.  This will need to hold at least
     *               `sample_count - pattern_sample_count` samples
     * @param sample_count Number of samples in the sample vector
     * @param pattern_sample_count Number of samples in the pattern vector
     * @param needs_conj True if the pattern vector is not already conjugated.  When enabled there is a
     *                   substantial performance penalty as this is done for each possible shift!
     * @return Number of valid values in the output vector
     */
    static uint32_t xcorr_in_place(const std::complex<float> * samples, const std::complex<float> * pattern,
                                   std::complex<float> * output,
                                   uint32_t sample_count, uint32_t pattern_sample_count, bool needs_conj);

    /**
     * See utils::xcorr_in_place
     * @param samples Vector of complex samples to search through
     * @param pattern Vector of complex samples to search for
     * @param needs_conj True if the pattern vector is not already conjugated
     * @return Vector of complex samples containing the results of the cross correlation
     */
    static std::vector<std::complex<float>> xcorr_vector(const std::vector<std::complex<float>> & samples,
                                                         const std::vector<std::complex<float>> & pattern,
                                                         bool needs_conj);

    /**
     * Compute the magnitude squared of each element in the input samples vector
     *
     * Formula is effectively: `pow(samples[idx].real(), 2) + pow(samples[idx].imag(), 2)`
     * @param samples Vector of complex samples
     * @param output Vector of floating point values
     * @param sample_count Number of samples in the input vector
     */
    static void mag_squared(const std::complex<float> * samples, float * output, uint32_t sample_count);

    /**
     * See utils::mag_squared
     * @param samples Vector of complex samples
     * @param output Vector of floating point values
     */
    static void mag_squared_vector_in_place(const std::vector<std::complex<float>> & samples, std::vector<float> & output);

    /**
     * See utils::mag_squared
     * @param samples Vector of complex samples
     * @return Vector of floating point values
     */
    static std::vector<float> mag_squared_vector(const std::vector<std::complex<float>> & samples);

    /**
     * Calculate the number of samples required to hold a full 9 OFDM symbol burst
     * @param sample_rate Sample rate (in Hz)
     * @return See above
     */
    static uint32_t get_burst_sample_count(float sample_rate);

    /**
     * Calculate the magnitude of the provided complex vector
     *
     * Formula is effectively: `sqrt(pow(samples[idx].real(), 2) + pow(samples[idx].imag(), 2))`
     * @param samples Vector of complex samples
     * @return Vector of floating point values
     */
    static std::vector<float> mag_vector(const std::vector<std::complex<float>> & samples);

    /**
     * Write complex samples to disk
     * @param path Path to store the complex samples
     * @param samples Vector of complex samples
     * @param sample_count Number of elements in the complex vector
     */
    static void write_samples(const std::string & path, const std::complex<float> * samples, uint32_t sample_count);

    /**
     * See utils::write_samples
     * @param path Path to store the complex samples
     * @param samples Vector of complex samples
     */
    static void write_samples_vector(const std::string & path, const std::vector<std::complex<float>> & samples);

    /**
     * Interpolate the input samples by the provided rate.  <b>Does not filter!</b>
     *
     * Interpolation is accomplished by stuffing `rate - 1` zeros between each sample
     * @param samples Vector of complex samples
     * @param rate Interpolation rate (must be > 0)
     * @return Vector of samples.size() * rate interpolated samples
     */
    static std::vector<std::complex<float>> interpolate(const std::vector<std::complex<float>> & samples, uint32_t rate);

    /**
     * Apply a filter to the provided sample vector
     * @param samples Vector of complex samples
     * @param taps Filter taps
     * @return Vector containing the filtered input samples
     */
    static std::vector<std::complex<float>> filter(const std::vector<std::complex<float>> & samples, const std::vector<float> & taps);
private:
};

} // namespace dji_droneid
} // namespace gr

#endif /* INCLUDED_DJI_DRONEID_UTILS_H */
