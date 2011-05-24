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

#ifndef B2NC_UTILS_H
#define B2NC_UTILS_H

#include <wreport/error.h>

namespace b2nc {

/**
 * Report an error message from the netcdf library. The message description
 * will be looked up using the netcdf library facilities.
 */
struct error_netcdf : public wreport::error
{
    std::string msg; ///< error message returned by what()

    error_netcdf(int ncerr, const std::string& msg);
    ~error_netcdf() throw () {}

    wreport::ErrorCode code() const throw () { return (wreport::ErrorCode)101; }

    virtual const char* what() const throw () { return msg.c_str(); }

    /// Throw the exception, building the message printf-style
    static void throwf(int ncerr, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);

    /**
     * Throw the exception, building the message printf-style, but only if
     * ncerr is an error
     */
    static void throwf_iferror(int ncerr, const char* fmt, ...) __attribute__ ((format(printf, 2, 3)));
};

}

#endif
