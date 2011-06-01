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

    static const char* DT_DATA;
    static const char* DT_QBITS;
    static const char* DT_CHAR;
    static const char* DT_QAINFO;

    static const char* ENV_TABLE_DIR;
    static const char* DEFAULT_TABLE_DIR;

    virtual ~Namer() {}

    /**
     * Mark the beginning of the section with the given tag
     */
    virtual void start(const std::string& tag) = 0;

    /**
     * Get a name for a variable
     *
     * @params code
     *   The variable code
     * @params tag
     *   A string tag identifying the portion of the input data where the code
     *   is found
     */
    virtual void name(const char* type, wreport::Varcode code, const std::string& tag, std::string& name, std::string& mnemo) = 0;

    /**
     * Get a namer by type.
     */
    static std::auto_ptr<Namer> get(Type type);
};


}

#endif

