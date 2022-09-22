/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_TIME_SYNC_H
#define INCLUDED_DRONEID_TIME_SYNC_H

#include <gnuradio/droneid/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace droneid {

/*!
 * \brief <+description of block+>
 * \ingroup droneid
 *
 */
class DRONEID_API time_sync : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<time_sync> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of droneid::time_sync.
     *
     * To avoid accidental use of raw pointers, droneid::time_sync's
     * constructor is in a private implementation
     * class. droneid::time_sync::make is the public interface for
     * creating new instances.
     */
    static sptr make(double sample_rate);
};

} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_TIME_SYNC_H */
