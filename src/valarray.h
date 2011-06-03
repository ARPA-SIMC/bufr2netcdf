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

struct LoopInfo
{
    std::string dimname;
    size_t firstarr;
    int nc_dimid;
    LoopInfo(const std::string& dimname, size_t firstarr)
        : dimname(dimname), firstarr(firstarr), nc_dimid(-1) {}
};


struct ValArray
{
    std::string name;
    std::string mnemo;
    wreport::Varinfo info;
    unsigned rcnt;
    int nc_varid;
    std::vector< std::pair<wreport::Varcode, std::string> > references;
    Namer::DataType type;
    ValArray* master;
    std::vector<ValArray*> slaves;
    ValArray* loop_var;
    // True if the value of the variable never changes
    bool is_constant;
    // Marker for newly created arrays, possibly still in need of
    // initialisation
    bool newly_created;

    ValArray(wreport::Varinfo info) : info(info), loop_var(0), is_constant(true), newly_created(true) {}
    virtual ~ValArray() {}
    virtual void add(const wreport::Var& var, unsigned bufr_idx) = 0;

    /// Returns the variable for the given BUFR and repetition instance
    virtual wreport::Var get_var(unsigned bufr_idx, unsigned rep=0) const = 0;

    /// Returns the array size (number of BUFR messages seen)
    virtual size_t get_size() const = 0;

    /// Returns the maximum number of repetition instances found
    virtual size_t get_max_rep() const = 0;

    virtual bool define(int ncid, int bufrdim) = 0;
    virtual void putvar(int ncid) const = 0;

    virtual void dump(FILE* out) = 0;

    static ValArray* make_singlevalarray(Namer::DataType type, wreport::Varinfo info);
    static ValArray* make_multivalarray(Namer::DataType type, wreport::Varinfo info, const LoopInfo& loopinfo);
};

}

#endif

