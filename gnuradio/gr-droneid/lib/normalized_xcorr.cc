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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <numeric>

#include <gnuradio/io_signature.h>
#include <droneid/normalized_xcorr.h>
#include <droneid/misc_utils.h>

#include <volk/volk.h>

namespace gr {
    namespace droneid {

        normalized_xcorr::normalized_xcorr(const complex_vec_t & filter_taps) {
            taps_ = misc_utils::conj(filter_taps);
            const auto mean = misc_utils::mean(taps_);
            std::for_each(taps_.begin(), taps_.end(), [&mean](complex_t & sample){
                sample -= mean;
            });

            taps_var_ = misc_utils::var(taps_);
            window_size_ = filter_taps.size();

            temp_.resize(window_size_);
        }

        normalized_xcorr::~normalized_xcorr() = default;

        uint32_t normalized_xcorr::run(const normalized_xcorr::complex_t * const samples, const uint32_t sample_count,
                                       normalized_xcorr::sample_t * const output_samples) {
            if (sample_count < window_size_) {
                return 0;
            }

            const auto max_correlations = sample_count - window_size_;
            if (scores_.size() < max_correlations) {
                scores_.resize(max_correlations);
            }

            for (auto offset = decltype(max_correlations){0}; offset < max_correlations; offset++) {
                volk_32fc_x2_dot_prod_32fc(&scores_[offset], samples + offset, &taps_[0], window_size_);
            }

            const complex_t scalar = {1 / static_cast<sample_t>(sqrt(taps_var_) * window_size_), 0};
            volk_32fc_s32fc_multiply_32fc(&scores_[0], &scores_[0], scalar, max_correlations);
            volk_32fc_magnitude_squared_32f(output_samples, &scores_[0], max_correlations);

            return max_correlations;
        }

    } /* namespace droneid */
} /* namespace gr */

