/*
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
#include "convert.h"
#include "utils.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wibble/regexp.h>
#include <wibble/string.h>
#include <wibble/sys/fs.h>
#include <netcdf.h>
#include <cstdlib>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct ncoutfile_shar
{
    ncoutfile_shar()
    {
    }

    ~ncoutfile_shar()
    {
    }
};
TESTGRP(ncoutfile);

static const char* testfname = "test-ncoutfile.nc";

// Test that ncid is -1 when the file is not open
template<> template<>
void to::test<1>()
{
    Options opts;
    NCOutfile out(opts);

    ensure_equals(out.ncid, -1);
    out.open(testfname);
    ensure(out.ncid != -1);
    out.close();
    ensure_equals(out.ncid, -1);
}

// Test that the file is created if missing
template<> template<>
void to::test<2>()
{
    Options opts;
    NCOutfile out(opts);

    sys::fs::deleteIfExists(testfname);

    out.open(testfname);
    out.close();

    ensure(sys::fs::exists(testfname));
}

}

// vim:set ts=4 sw=4:
