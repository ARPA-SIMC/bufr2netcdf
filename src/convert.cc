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

#include "convert.h"
#include "arrays.h"
#include "utils.h"
#include <wreport/bulletin.h>
#include <map>
#include <vector>
#include <netcdf.h>

using namespace wreport;
using namespace std;

namespace b2nc {

void Converter::convert(FILE* in, int outncid)
{
    string rawmsg;
    BufrBulletin bulletin;

    Arrays arrays;
    while (BufrBulletin::read(in, rawmsg /* , fname = 0 */))
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);
        // TODO: if first, build metadata
        // Add contents to the various data arrays
        arrays.add(bulletin);
    }

    // TODO: add arrays to NetCDF
    int res;

    int dim_bufr_records;
    res = nc_def_dim(outncid, "BUFR_records", NC_UNLIMITED, &dim_bufr_records);
    error_netcdf::throwf_iferror(res, "creating BUFR_records dimension for file %s", "##TODO##");

    for (std::vector<ValArray*>::const_iterator i = arrays.arrays.begin();
            i != arrays.arrays.end(); ++i)
    {
        /* int id = */ (*i)->define(outncid, dim_bufr_records);
    }

    // TODO nc_put_att       /* put attribute: assign attribute values */

    // End define mode
    res = nc_enddef(outncid);
    error_netcdf::throwf_iferror(res, "leaving define mode for file %s", "##TODO##");

    for (std::vector<ValArray*>::const_iterator i = arrays.arrays.begin();
            i != arrays.arrays.end(); ++i)
    {
        (*i)->putvar(outncid);
    }
}

}
