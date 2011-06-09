/*
 * valarray - Storage for plain variable values
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

#ifndef B2NC_VALARRAY_H
#define B2NC_VALARRAY_H

#include "namer.h"
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <cstdio>

namespace wreport {
struct Var;
}

namespace b2nc {

struct NCOutfile;
struct ValArray;

struct LoopInfo
{
    /**
     * Pointer to variable holding the delayed replication count for this
     * section's loop, if any
     */
    ValArray* var;

    /**
     * Index of the loop, used to name Loop_NNN_maxlen attributes
     */
    unsigned index;

    /// NetCDF dimension ID of the loop dimension
    int nc_dimid;

    LoopInfo()
        : var(0), index(0), nc_dimid(-1) {}

    void define(NCOutfile& outfile, size_t size);
};

struct ValArray
{
    std::string name;
    std::string mnemo;
    wreport::Varinfo info;
    unsigned rcnt;
    int nc_varid;
    std::vector< std::pair<wreport::Varcode, const ValArray*> > references;
    Namer::DataType type;
    ValArray* master;
    std::vector<ValArray*> slaves;
    // True if the value of the variable never changes
    bool is_constant;

    ValArray(wreport::Varinfo info);
    virtual ~ValArray() {}
    virtual void add(const wreport::Var& var, unsigned bufr_idx) = 0;

    /// Returns the variable for the given BUFR and repetition instance
    virtual wreport::Var get_var(unsigned bufr_idx, unsigned rep=0) const = 0;

    /// Returns the array size (number of BUFR messages seen)
    virtual size_t get_size() const = 0;

    /// Returns the maximum number of repetition instances found
    virtual size_t get_max_rep() const = 0;

    /// Returns true if the array contains some defined values, false if it's
    /// all undefined values
    virtual bool has_values() const = 0;

    virtual bool define(NCOutfile& outfile) = 0;
    virtual void putvar(NCOutfile& outfile) const = 0;

    virtual void dump(FILE* out) = 0;

    static ValArray* make_singlevalarray(Namer::DataType type, wreport::Varinfo info);
    static ValArray* make_multivalarray(Namer::DataType type, wreport::Varinfo info, LoopInfo& loopinfo);
};

}

#endif

