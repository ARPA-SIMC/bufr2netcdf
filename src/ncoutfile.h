/*
 * ncoutfile - NetCDF output file
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

#ifndef B2NC_NCOUTFILE_H
#define B2NC_NCOUTFILE_H

#include <string>

namespace wreport {
struct BufrBulletin;
}

namespace b2nc {

struct Options;

/**
 * One output NetCDF file
 */
struct NCOutfile
{
public:
    std::string fname;
    int ncid;
    int dim_bufr_records;

    NCOutfile(const Options& opts);
    ~NCOutfile();

    // Open the NetCDF file for writing, and initialise common parts
    void open(const std::string& fname);

    /**
     * Finalise and close the file
     *
     * This sets ncid to -1, which you can test to see if the file already has
     * been closed.
     */
    void close();

    /**
     * End NetCDF define mode
     */
    void end_define_mode();
};

}

#endif
