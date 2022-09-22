/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_DEMODULATION_H
#define INCLUDED_DRONEID_DEMODULATION_H

#include <gnuradio/droneid/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace droneid {

/*!
 * \brief <+description of block+>
 * \ingroup droneid
 *
 */
class DRONEID_API demodulation : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<demodulation> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of droneid::demodulation.
     *
     * To avoid accidental use of raw pointers, droneid::demodulation's
     * constructor is in a private implementation
     * class. droneid::demodulation::make is the public interface for
     * creating new instances.
     */
    static sptr make(double sample_rate);
};

} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_DEMODULATION_H */
