/*
 * namers - Naming stategies for NetCDF data arrays
 *
 * Copyright (C) 2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef B2NC_NAMERS_H
#define B2NC_NAMERS_H

#include <wreport/varinfo.h>
#include <string>
#include <memory>

namespace b2nc {

struct Namer
{
    enum Type {
        PLAIN,
        MNEMONIC,
    };

    virtual ~Namer() {}

    /**
     * Get a name for a variable
     *
     * @params code
     *   The variable code
     * @params nested
     *   True if the variable is seen inside a nested loop, false if it is at
     *   the top level of the BUFR message
     */
    virtual std::string name(wreport::Varcode code, bool nested=false) = 0;

    /**
     * Get a namer by type.
     */
    static std::auto_ptr<Namer> get(Type type);
};


}

#endif
