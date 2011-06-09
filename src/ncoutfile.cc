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

#include "ncoutfile.h"
#include "utils.h"
#include <cstdio>

using namespace wreport;
using namespace std;

namespace b2nc {

NCOutfile::NCOutfile(const Options& opts)
    : ncid(-1), dim_bufr_records(-1) {}

NCOutfile::~NCOutfile()
{
    close();
}

void NCOutfile::open(const std::string& fname)
{
    // Create output file
    this->fname = fname;
    int res = nc_create(fname.c_str(), NC_CLOBBER, &ncid);
    error_netcdf::throwf_iferror(res, "creating file %s", fname.c_str());

    // Define BUFR_records dimension, which is always present and UNLIMITED
    res = nc_def_dim(ncid, "BUFR_records", NC_UNLIMITED, &dim_bufr_records);
    error_netcdf::throwf_iferror(res, "creating BUFR_records dimension for file %s", fname.c_str());

}

void NCOutfile::close()
{
    if (ncid == -1)
        return;

    // Close file
    int res = nc_close(ncid);
    error_netcdf::throwf_iferror(res, "closing file %s", fname.c_str());
    ncid = -1;
}

void NCOutfile::end_define_mode()
{
    int res = nc_enddef(ncid);
    error_netcdf::throwf_iferror(res, "leaving define mode for file %s", fname.c_str());
}

int NCOutfile::def_var(const char* name, nc_type xtype, int ndims, const int *dimidsp)
{
    int varid;
    int res = nc_def_var(ncid, name, xtype, ndims, dimidsp, &varid);
    error_netcdf::throwf_iferror(res, "creating variable %s", name);
    return varid;
}

}
