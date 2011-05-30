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
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {

void Converter::convert(FILE* in, int outncid)
{
    string rawmsg;
    BufrBulletin bulletin;

    vector<string> opt_sections;
    unsigned max_os_len = 0;

    Arrays arrays;
    while (BufrBulletin::read(in, rawmsg /* , fname = 0 */))
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);
        // TODO: if first, build metadata
        // Add contents to the various data arrays
        arrays.add(bulletin);

        if (bulletin.optional_section_length)
        {
            if (bulletin.optional_section_length > (int)max_os_len)
                max_os_len = bulletin.optional_section_length;
            opt_sections.push_back(string(bulletin.optional_section, bulletin.optional_section_length));
        }
        else
            opt_sections.push_back(string());
    }

    // TODO: add arrays to NetCDF
    int res;

    // Define dimensions

    int dim_bufr_records;
    res = nc_def_dim(outncid, "BUFR_records", NC_UNLIMITED, &dim_bufr_records);
    error_netcdf::throwf_iferror(res, "creating BUFR_records dimension for file %s", "##TODO##");

    int dim_sec2;
    res = nc_def_dim(outncid, "section_2_length", max_os_len, &dim_sec2);
    error_netcdf::throwf_iferror(res, "creating section_2_length dimension for file %s", "##TODO##");

    // Define variables

    for (std::vector<ValArray*>::const_iterator i = arrays.arrays.begin();
            i != arrays.arrays.end(); ++i)
    {
        /* int id = */ (*i)->define(outncid, dim_bufr_records);
    }

    int sec2_varid;
    {
        int dims[] = { dim_bufr_records, dim_sec2 };
        res = nc_def_var(outncid, "section2", NC_BYTE, 2, dims, &sec2_varid);
        error_netcdf::throwf_iferror(res, "creating variable section2");
    }

    // TODO nc_put_att       /* put attribute: assign attribute values */

    // End define mode
    res = nc_enddef(outncid);
    error_netcdf::throwf_iferror(res, "leaving define mode for file %s", "##TODO##");

    {
        size_t start[] = {0, 0};
        size_t count[] = {1, 0};
        unsigned char missing[max_os_len]; // Missing value
        memset(missing, NC_FILL_BYTE, max_os_len);
        for (size_t i = 0; i < opt_sections.size(); ++i)
        {
            int res;
            start[0] = i;
            if (!opt_sections[i].empty())
            {
                count[1] = opt_sections[i].size();
                res = nc_put_vara_uchar(outncid, sec2_varid, start, count, (const unsigned char*)opt_sections[i].data());
            }
            else
            {
                count[1] = max_os_len;
                res = nc_put_vara_uchar(outncid, sec2_varid, start, count, missing);
            }
            error_netcdf::throwf_iferror(res, "storing %zd string values", count[1]);
        }
    }

    for (std::vector<ValArray*>::const_iterator i = arrays.arrays.begin();
            i != arrays.arrays.end(); ++i)
    {
        (*i)->putvar(outncid);
    }
}

}
