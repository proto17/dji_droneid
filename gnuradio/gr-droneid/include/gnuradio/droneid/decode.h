/* -*- c++ -*- */
/*
 * Copyright 2022 gr-droneid author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_DRONEID_DECODE_H
#define INCLUDED_DRONEID_DECODE_H

#include <gnuradio/droneid/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace droneid {

/*!
 * \brief <+description of block+>
 * \ingroup droneid
 *
 */
class DRONEID_API decode : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<decode> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of droneid::decode.
     *
     * To avoid accidental use of raw pointers, droneid::decode's
     * constructor is in a private implementation
     * class. droneid::decode::make is the public interface for
     * creating new instances.
     */
    static sptr make(const std::string & debug_path);
};

} // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_DECODE_H */
