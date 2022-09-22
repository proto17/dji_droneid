//
// Created by main on 4/20/22.
//

#include "utils.h"
#include <gnuradio/filter/fft_filter.h>

uint32_t get_long_cp_len(double sample_rate) {
    return static_cast<uint32_t>(std::round(sample_rate / 192000));
}

uint32_t get_short_cp_len(double sample_rate) {
    return static_cast<uint32_t>(round(0.0000046875 * sample_rate));
}

uint32_t get_fft_size(double sample_rate) {
    return static_cast<uint32_t>(round(sample_rate / CARRIER_SPACING));
}

std::vector<std::complex<float>> create_zc_sequence(const double sample_rate, const uint32_t root) {
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
    sequence[(fft_size / 2) - 1] = 0;

    // Create an FFT object that is configured to run an inverse FFT
    gr::fft::fft_complex_rev ifft(static_cast<int>(fft_size), 1);

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
