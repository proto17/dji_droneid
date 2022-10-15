/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DJI_DRONEID_DETECTOR_H
#define INCLUDED_DJI_DRONEID_DETECTOR_H

#include <gnuradio/dji_droneid/api.h>
#include <gnuradio/hier_block2.h>

namespace gr {
namespace dji_droneid {

/*!
 * \brief <+description of block+>
 * \ingroup dji_droneid
 *
 */
class DJI_DRONEID_API detector : virtual public gr::hier_block2
{
public:
    typedef std::shared_ptr<detector> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of dji_droneid::detector.
     *
     * To avoid accidental use of raw pointers, dji_droneid::detector's
     * constructor is in a private implementation
     * class. dji_droneid::detector::make is the public interface for
     * creating new instances.
     */
    static sptr make(float sample_rate);
};

} // namespace dji_droneid
} // namespace gr

#endif /* INCLUDED_DJI_DRONEID_DETECTOR_H */
