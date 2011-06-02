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

#include <string>
#include <memory>
#include <cstdio>

namespace wreport {
struct BufrBulletin;
}

namespace b2nc {

/**
 * Configuration for the conversion process
 */
struct Options
{
    bool use_mnemonic;

    Options()
        : use_mnemonic(true)
    {
    }
};

/**
 * One output NetCDF file
 */
struct Outfile
{
public:
    std::string fname;
    int ncid;

    Outfile();
    virtual ~Outfile();

    /**
     * Start writing to the given output file, overwriting it if it already exists
     */
    void open(const std::string& fname);

    /**
     * Write all data to the output file and close it
     */
    virtual void close();

    /**
     * Add all the contents of the given BUFR file
     */
    void add_bufr(const std::string& fname);

    /**
     * Add all the condents of the given BUFR stream
     *
     * @param file
     *   if provided, it is used as the file name in error messages
     */
    void add_bufr(FILE* in, const char* fname = 0);

    /**
     * Add all the contents of the decoded BUFR message
     */
    virtual void add_bufr(const wreport::BufrBulletin& bulletin) = 0;

    /**
     * Create an Outfile
     */
    static std::auto_ptr<Outfile> get(const Options& opts);
};

}

#endif
