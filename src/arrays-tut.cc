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
#include "options.h"
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
    Options opts; opts.use_mnemonic = false;
    Arrays arrays(opts);
    read_bufr(arrays, "cdfin_acars");

    Plan& p = arrays.plan;

    ensure(p.get_section(0));
    ensure(p.get_section(1));

    ensure_equals(p.get_section(0)->entries.size(), 41);
    ensure_equals(p.get_section(1)->entries.size(), 2);

    const plan::Variable* v;

    v = p.get_variable(0, 0);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B01033_000");
    ensure_equals(v->data->get_size(), 133);
    ensure_equals(v->data->get_max_rep(), 1);

    v = p.get_variable(0, 4);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B05001_000");
    ensure_equals(v->data->get_size(), 133);
    ensure_equals(v->data->get_max_rep(), 1);
    Var var = v->data->get_var(0, 0);
    ensure_equals(var.enqd(), 48.45500);
    var = v->data->get_var(132, 0);
    ensure_equals(var.enqd(), 46.16833);

    v = p.get_variable(0, 40);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B33026_000");
    ensure_equals(v->data->get_size(), 133);
    ensure_equals(v->data->get_max_rep(), 1);

    v = p.get_variable(1, 0);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B11075_000");
    ensure_equals(v->data->get_size(), 0);
    ensure_equals(v->data->get_max_rep(), 0);

    //p.print(stderr);
}

template<> template<>
void to::test<2>()
{
    Options opts; opts.use_mnemonic = false;
    Arrays arrays(opts);
    read_bufr(arrays, "cdfin_gps_zenith");

    Plan& p = arrays.plan;

    ensure(p.get_section(0));
    ensure(p.get_section(1));

    ensure_equals(p.get_section(0)->entries.size(), 26);
    ensure_equals(p.get_section(1)->entries.size(), 6);

    const plan::Variable* v;

    v = p.get_variable(0, 0);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B01015_000");
    ensure_equals(v->data->get_size(), 94u);
    ensure_equals(v->data->get_max_rep(), 1);

    v = p.get_variable(1, 0);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B02020_000");
    ensure_equals(v->data->get_size(), 94u);
    ensure_equals(v->data->get_max_rep(), 25u);
    Var var = v->data->get_var(0, 0);
    ensure(!var.isset());
    var = v->data->get_var(93, 24);
    ensure(!var.isset());
    var = v->data->get_var(94, 25);
    ensure(!var.isset());

    v = p.get_variable(1, 2);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B05021_000");
    ensure_equals(v->data->get_size(), 94u);
    ensure_equals(v->data->get_max_rep(), 25u);

    v = p.get_variable(0, 25);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B15011_000");
    ensure_equals(v->data->get_size(), 94u);
    ensure_equals(v->data->get_max_rep(), 1);

    //p.print(stderr);
}

template<> template<>
void to::test<3>()
{
    Options opts; opts.use_mnemonic = false;
    Arrays arrays(opts);
    read_bufr(arrays, "cdfin_buoy");

    Plan& p = arrays.plan;

    ensure(p.get_section(0));
    ensure(p.get_section(1));

    ensure_equals(p.get_section(0)->entries.size(), 106);
    ensure_equals(p.get_section(1)->entries.size(), 3);
    ensure_equals(p.get_section(2)->entries.size(), 3);
    ensure_equals(p.get_section(3)->entries.size(), 1);
    ensure_equals(p.get_section(4)->entries.size(), 1);

    const plan::Variable* v;

    v = p.get_variable(0, 0);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Data_B31021_000");
    ensure_equals(v->data->get_size(), 13u);
    ensure_equals(v->data->get_max_rep(), 1);

    v = p.get_variable(4, 0);
    ensure(v);
    ensure(v->data);
    ensure_equals(v->data->name, "Char_C05008_000");
    ensure_equals(v->data->get_size(), 13u);
    ensure_equals(v->data->get_max_rep(), 2);
    Var var = v->data->get_var(0, 0);
    ensure(var.isset());
    ensure_equals(var.enq<std::string>(), "22219 61");
    var = v->data->get_var(0, 1);
    ensure(var.isset());
    ensure_equals(var.enq<std::string>(), "12/     ");

    //p.print(stderr);

}

}

// vim:set ts=4 sw=4:

