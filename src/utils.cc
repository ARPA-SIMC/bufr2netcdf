/*
 * utils - Miscellaneous utility functions
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

#include "utils.h"
#include <netcdf.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace b2nc {

error_netcdf::error_netcdf(int ncerr, const std::string& msg)
{
    this->msg = msg + ": " + nc_strerror(ncerr);
}

void error_netcdf::throwf(int ncerr, const char* fmt, ...)
{
    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    char* cmsg;
    vasprintf(&cmsg, fmt, ap);
    va_end(ap);

    // Convert to string
    std::string msg(cmsg);
    free(cmsg);
    throw error_netcdf(ncerr, msg);
}

void error_netcdf::throwf_iferror(int ncerr, const char* fmt, ...)
{
    if (ncerr == NC_NOERR) return;

    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    char* cmsg;
    vasprintf(&cmsg, fmt, ap);
    va_end(ap);

    // Convert to string
    std::string msg(cmsg);
    free(cmsg);
    throw error_netcdf(ncerr, msg);
}

}
