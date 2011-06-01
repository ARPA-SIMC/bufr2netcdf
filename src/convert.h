/*
 * convert - Convert a stream of BUFR messages to one single NetCDF file
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

#ifndef B2NC_CONVERT_H
#define B2NC_CONVERT_H

#include <cstdio>
#include <string>

namespace b2nc {

/**
 * Configurable BUFR to NetCDF converter
 */
class Converter
{
public:

    /**
     * Convert a stream of BUFR messages to one NetCDF file
     */
    void convert(FILE* in, int outncid);
};

struct Outfile
{
public:
    std::string fname;
    int ncid;

    Outfile();
    Outfile(const std::string& fname);
    ~Outfile();

    void open(const std::string& fname);
    void close();

    void add_bufr(const std::string& fname);
    void add_bufr(FILE* in);
};

}

#endif
