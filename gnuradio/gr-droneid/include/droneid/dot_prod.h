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

#ifndef INCLUDED_DRONEID_DOT_PROD_H
#define INCLUDED_DRONEID_DOT_PROD_H

#include <droneid/api.h>
#include <gnuradio/block.h>

namespace gr {
    namespace droneid {

        /*!
         * \brief <+description of block+>
         * \ingroup droneid
         *
         */
        class DRONEID_API dot_prod : virtual public gr::block {
        public:
            typedef boost::shared_ptr<dot_prod> sptr;

            /*!
             * \brief Return a shared_ptr to a new instance of droneid::dot_prod.
             *
             * To avoid accidental use of raw pointers, droneid::dot_prod's
             * constructor is in a private implementation
             * class. droneid::dot_prod::make is the public interface for
             * creating new instances.
             */
            static sptr make(const std::vector<gr_complex> & /*taps*/);
        };

    } // namespace droneid
} // namespace gr

#endif /* INCLUDED_DRONEID_DOT_PROD_H */

