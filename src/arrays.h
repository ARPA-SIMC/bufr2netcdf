/*
 * arrays - Aggregate multiple BUFR contents into data arrays
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

#ifndef B2NC_ARRAYS_H
#define B2NC_ARRAYS_H

#include "namer.h"
#include "valarray.h"
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <map>
#include <cstdio>

namespace wreport {
struct Var;
struct Bulletin;
struct BufrBulletin;
struct Subset;
}

namespace b2nc {

struct Arrays
{
    Namer* namer;
    std::vector<ValArray*> arrays;
    std::map<std::string, unsigned> byname;
    // Map loop tags to dimension information
    std::map<std::string, LoopInfo> dimnames;

    // Pointers used for date/time aggregations (NULL if not found)
    ValArray* date_year;
    ValArray* date_month;
    ValArray* date_day;
    ValArray* time_hour;
    ValArray* time_minute;
    ValArray* time_second;

    int date_varid;
    int time_varid;

    unsigned loop_idx;
    int bufr_idx;

    Arrays(Namer::Type type = Namer::PLAIN);
    ~Arrays();

    void start(const std::string& tag);
    ValArray& get_valarray(const char* type, const wreport::Var& var, const std::string& tag);

    /**
     * Adds all the subsets for a bulletin.
     */
    void add(const wreport::Bulletin& bulletin);

    bool define(int ncid, int bufrdim);
    void putvar(int ncid) const;

    void dump(FILE* out);
};

struct Sections
{
    std::vector<std::string> values;
    unsigned max_length;
    unsigned idx;
    int nc_dimid;
    int nc_varid;

    Sections(unsigned idx);

    void add(const wreport::BufrBulletin& bulletin);

    bool define(int ncid, int bufrdim);
    void putvar(int ncid) const;
};

struct IntArray
{
    std::string name;
    std::vector<int> values;
    int nc_varid;

    IntArray(const std::string& name);

    void add(int val);
    void add_missing();

    bool define(int ncid, int bufrdim);
    void putvar(int ncid) const;
};

}

#endif

