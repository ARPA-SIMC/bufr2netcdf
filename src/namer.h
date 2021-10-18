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

struct Options;

/**
 * Compute NetCDF variable names for their corresponding BUFR variables.
 *
 * This is an abstract interface: implementations are in namer.cc and chosen
 * depending on configuration.
 */
struct Namer
{
    enum Type {
        PLAIN,
        MNEMONIC,
    };

    enum DataType {
        DT_DATA   = 0,
        DT_QBITS  = 1,
        DT_CHAR   = 2,
        DT_QAINFO = 3,
        DT_MAX    = 4,
    };

    static const char* ENV_TABLE_DIR;
    static const char* DEFAULT_TABLE_DIR;

    virtual ~Namer() {}

    /**
     * Get a name for a variable
     *
     * @params code
     *   The variable code
     * @params tag
     *   A string tag identifying the portion of the input data where the code
     *   is found
     * @returns the number of time the given code has been seen in the message,
     *   excluding repetitions
     */
    virtual unsigned name(DataType type, wreport::Varcode code, size_t tag, std::string& name, std::string& mnemo) = 0;

    /**
     * Get a namer by type.
     */
    static std::unique_ptr<Namer> get(const Options& opts);

    /**
     * Get the string name for a type
     */
    static const char* type_name(DataType type);
};


}

#endif

