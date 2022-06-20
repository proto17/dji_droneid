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

#ifndef INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_H
#define INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_H

#include <droneid/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace droneid {

    /*!
     * \brief <+description of block+>
     * \ingroup droneid
     *
     */
    class DRONEID_API normalized_xcorr_estimate : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<normalized_xcorr_estimate> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of droneid::normalized_xcorr_estimate.
       *
       * To avoid accidental use of raw pointers, droneid::normalized_xcorr_estimate's
       * constructor is in a private implementation
       * class. droneid::normalized_xcorr_estimate::make is the public interface for
       * creating new instances.
       */
      static sptr make(std::vector<std::complex<float>> filter_taps);
    };

  } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_H */

