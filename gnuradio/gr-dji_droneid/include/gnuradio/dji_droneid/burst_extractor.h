/* -*- c++ -*- */
/*
 * Copyright 2022 gr-dji_droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DJI_DRONEID_BURST_EXTRACTOR_H
#define INCLUDED_DJI_DRONEID_BURST_EXTRACTOR_H

#include <gnuradio/dji_droneid/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace dji_droneid {

/*!
 * \brief <+description of block+>
 * \ingroup dji_droneid
 *
 */
class DJI_DRONEID_API burst_extractor : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<burst_extractor> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of dji_droneid::burst_extractor.
     *
     * To avoid accidental use of raw pointers, dji_droneid::burst_extractor's
     * constructor is in a private implementation
     * class. dji_droneid::burst_extractor::make is the public interface for
     * creating new instances.
     */
    static sptr make(float sample_rate, float threshold);

    virtual void set_threshold(float threshold) = 0;

    virtual pmt::pmt_t get_output_port_name() const = 0;
};

} // namespace dji_droneid
} // namespace gr

#endif /* INCLUDED_DJI_DRONEID_BURST_EXTRACTOR_H */
