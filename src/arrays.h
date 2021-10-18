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
#include "plan.h"
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

struct Options;
struct NCOutfile;

/**
 * Constructs and holds NetCDF arrays from BUFR bulletins
 */
struct Arrays
{
    // Coversion strategy object that also holds the variables that are being
    // built
    Plan plan;

    // Pointers used for date/time aggregations (NULL if not found)
    ValArray* date_year;
    ValArray* date_month;
    ValArray* date_day;
    ValArray* time_hour;
    ValArray* time_minute;
    ValArray* time_second;

    int date_varid;
    int time_varid;

    unsigned bufr_idx = 0;

    bool verbose;
    bool debug;

    /*
     * Keep a copy of the first bulletin that we use to generate the plan, as
     * the plan's temporay Varinfos will be managed by it
     */
    wreport::Bulletin* first_bulletin = 0;

    Arrays(const Options& opts);
    ~Arrays();

    /**
     * Adds all the subsets for a bulletin.
     */
    void add(std::unique_ptr<wreport::Bulletin>&& bulletin);

    /**
     * Define variables for these arrays on a NetCDF file in define mode
     */
    bool define(NCOutfile& outfile);

    /**
     * Write the values of the various NetCDF variables that were colleced, to
     * a NetCDF file in data mode
     */
    void putvar(NCOutfile& outfile) const;

    void dump(FILE* out);
};

/// Array for binary dumps of BUFR sections
struct Sections
{
    std::vector<std::string> values;
    unsigned max_length;
    unsigned idx;
    int nc_dimid;
    int nc_varid;

    Sections(unsigned idx);

    void add(const wreport::BufrBulletin& bulletin, const std::string& raw);

    bool define(NCOutfile& outfile);
    void putvar(NCOutfile& outfile) const;
};

/**
 * Integer NetCDF variable, used to store values from BUFR metadata
 */
struct IntArray
{
    std::string name;
    std::vector<int> values;
    int nc_varid;

    IntArray(const std::string& name);

    void add(int val);
    void add_missing();

    bool define(NCOutfile& outfile);
    void putvar(NCOutfile& outfile) const;
};

}

#endif

