/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_H
#define INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_H

#include <gnuradio/droneid/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace droneid {

/*!
 * \brief <+description of block+>
 * \ingroup droneid
 *
 */
class DRONEID_API normalized_xcorr_estimate : virtual public gr::block
{
public:
    typedef std::shared_ptr<normalized_xcorr_estimate> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of droneid::normalized_xcorr_estimate.
     *
     * To avoid accidental use of raw pointers, droneid::normalized_xcorr_estimate's
     * constructor is in a private implementation
     * class. droneid::normalized_xcorr_estimate::make is the public interface for
     * creating new instances.
     */
    static sptr make(const std::vector<gr_complex> & /*taps*/);
};

} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_NORMALIZED_XCORR_ESTIMATE_H */
