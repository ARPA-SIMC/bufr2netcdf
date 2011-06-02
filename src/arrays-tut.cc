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

#include "arrays.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wreport/bulletin.h>
#include <cstdio>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct arrays_shar
{
    arrays_shar()
    {
    }

    ~arrays_shar()
    {
    }
};
TESTGRP(arrays);

static void read_bufr(Arrays& a, const std::string& testname)
{
    // Open input file
    string srcfile(b2nc::tests::datafile("bufr/" + testname));
    FILE* infd = fopen(srcfile.c_str(), "rb");
    if (infd == NULL)
        error_system::throwf("cannot open %s", srcfile.c_str());

    string rawmsg;
    BufrBulletin bulletin;
    while (BufrBulletin::read(infd, rawmsg, srcfile.c_str()))
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);
        // Add contents to the various data arrays
        a.add(bulletin);
    }

    fclose(infd);
}

template<> template<>
void to::test<1>()
{
    Arrays arrays;
    read_bufr(arrays, "cdfin_acars");

    ensure_equals(arrays.arrays.size(), 40u);
    ensure_equals(arrays.arrays[0]->name, "Data_B01033_000");
    ensure_equals(arrays.arrays[0]->get_size(), 133);
    ensure_equals(arrays.arrays[0]->get_max_rep(), 1);
    ensure_equals(arrays.arrays[4]->name, "Data_B05001_000");
    ensure_equals(arrays.arrays[4]->get_size(), 133);
    ensure_equals(arrays.arrays[4]->get_max_rep(), 1);
    ensure_equals(arrays.arrays[39]->name, "Data_B33026_000");
    ensure_equals(arrays.arrays[39]->get_size(), 133);
    ensure_equals(arrays.arrays[39]->get_max_rep(), 1);

    Var var = arrays.arrays[4]->get_var(0, 0);
    ensure_equals(var.enqd(), 48.45500);

    var = arrays.arrays[4]->get_var(132, 0);
    ensure_equals(var.enqd(), 46.16833);

    //arrays.dump(stderr);
}

template<> template<>
void to::test<2>()
{
    Arrays arrays;
    read_bufr(arrays, "cdfin_gps_zenith");

    ensure_equals(arrays.arrays.size(), 31u);
    ensure_equals(arrays.arrays[0]->name, "Data_B01015_000");
    ensure_equals(arrays.arrays[0]->get_size(), 94u);
    ensure_equals(arrays.arrays[0]->get_max_rep(), 1);
    ensure_equals(arrays.arrays[16]->name, "Data_B02020_000");
    ensure_equals(arrays.arrays[16]->get_size(), 94u);
    ensure_equals(arrays.arrays[16]->get_max_rep(), 25u);
    ensure_equals(arrays.arrays[18]->name, "Data_B05021_000");
    ensure_equals(arrays.arrays[18]->get_size(), 94u);
    ensure_equals(arrays.arrays[18]->get_max_rep(), 25u);
    ensure_equals(arrays.arrays[30]->name, "Data_B15011_000");
    ensure_equals(arrays.arrays[30]->get_size(), 94u);
    ensure_equals(arrays.arrays[30]->get_max_rep(), 1);

    Var var = arrays.arrays[16]->get_var(0, 0);
    ensure(!var.isset());

    var = arrays.arrays[16]->get_var(93, 24);
    ensure(!var.isset());

    var = arrays.arrays[16]->get_var(94, 25);
    ensure(!var.isset());

    var = arrays.arrays[18]->get_var(0, 0);
    ensure(var.isset());
    ensure_equals(var.enqi(), 0);

    //arrays.dump(stderr);
}

}

// vim:set ts=4 sw=4:

