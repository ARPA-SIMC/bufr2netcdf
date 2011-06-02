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

#include <wreport/varinfo.h>
#include <string>
#include <memory>
#include <map>
#include <set>
#include <vector>
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
    std::string out_fname;

    Options()
        : use_mnemonic(true)
    {
    }
};

/// Generic interface for BUFR consumers
struct BufrSink
{
    virtual ~BufrSink() {}
    virtual void add_bufr(const wreport::BufrBulletin& bulletin) = 0;
};

/**
 * Send all the contents of the given BUFR file to \a out
 */
void read_bufr(const std::string& fname, BufrSink& out);

/**
 * Send all the condents of the given BUFR stream to \a out
 *
 * @param file
 *   if provided, it is used as the file name in error messages
 */
void read_bufr(FILE* in, BufrSink& out, const char* fname = 0);


/**
 * One output NetCDF file
 */
struct Outfile : public BufrSink
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
     * Add all the contents of the decoded BUFR message
     */
    virtual void add_bufr(const wreport::BufrBulletin& bulletin) = 0;

    /**
     * Create an Outfile
     */
    static std::auto_ptr<Outfile> get(const Options& opts);
};

/**
 * Dispatch BUFR messages to different Outfiles based on their Data Descriptor
 * Sections.
 *
 * All bufr sent to an Outfile will have exactly the same DDS.
 */
class Dispatcher : public BufrSink
{
protected:
    const Options& opts;
    std::map< std::vector<wreport::Varcode>, Outfile* > outfiles;
    std::set<std::string> used_fnames;

    std::string get_fname(const wreport::BufrBulletin& bulletin);
    Outfile& get_outfile(const wreport::BufrBulletin& bulletin);

public:
    Dispatcher(const Options& opts);
    virtual ~Dispatcher();

    void close();

    virtual void add_bufr(const wreport::BufrBulletin& bulletin);
};

}

#endif
