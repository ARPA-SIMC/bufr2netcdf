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

#include "valarray.h"
#include "options.h"
#include "utils.h"
#include "ncoutfile.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wreport/bulletin.h>
#include <cstdio>
#include <netcdf.h>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

static const char* testfname = "test-valarray.nc";

struct valarray_shar
{
    const Vartable* table;

    valarray_shar()
    {
        table = Vartable::get("B0000000000000014000");
        ensure(table);
    }

    ~valarray_shar()
    {
    }
};
TESTGRP(valarray);

template<> template<>
void to::test<1>()
{
    Var var(table->query(WR_VAR(0, 1, 1)));
    auto_ptr<ValArray> arr(ValArray::make_singlevalarray(Namer::DT_DATA, var.info()));
    // TODO
}

// Test exporting string arrays
template<> template<>
void to::test<2>()
{
    // Loop variable (simulate delayed replication)
    LoopInfo loopinfo;
    // Multidimensional variable
    Var var(table->query(WR_VAR(0, 1, 15)));
    auto_ptr<ValArray> arr(ValArray::make_multivalarray(Namer::DT_DATA, var.info(), loopinfo));
    arr->name = "TEST";
    arr->mnemo = "TEST";
    arr->rcnt = 0;
    arr->type = Namer::DT_DATA;

    // Add a 2x2 matrix of values
    var.set("val0.0");
    arr->add(var, 0);
    var.set("val0.1");
    arr->add(var, 0);
    var.set("val1.0");
    arr->add(var, 1);
    var.set("val1.1");
    arr->add(var, 1);

    // Export to a NetCDF file
    Options opts;
    NCOutfile outfile(opts);
    outfile.open(testfname);

    arr->define(outfile);
    outfile.end_define_mode();
    arr->putvar(outfile);

    int res;

    size_t start[] = {0, 0, 0};
    size_t count[] = {1, 1, 6};
    char buf[7];

    res = nc_get_vara_text(outfile.ncid, arr->nc_varid, start, count, buf);
    error_netcdf::throwf_iferror(res, "reading variable from %s", testfname);
    buf[6] = 0;
    ensure_equals(string(buf), "val0.0");

    start[1] = 1;
    res = nc_get_vara_text(outfile.ncid, arr->nc_varid, start, count, buf);
    error_netcdf::throwf_iferror(res, "reading variable from %s", testfname);
    buf[6] = 0;
    ensure_equals(string(buf), "val0.1");

    start[0] = 1;
    start[1] = 0;
    res = nc_get_vara_text(outfile.ncid, arr->nc_varid, start, count, buf);
    error_netcdf::throwf_iferror(res, "reading variable from %s", testfname);
    buf[6] = 0;
    ensure_equals(string(buf), "val1.0");

    outfile.close();
}

}

// vim:set ts=4 sw=4:

