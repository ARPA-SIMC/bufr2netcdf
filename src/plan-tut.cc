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

#include "plan.h"
#include "options.h"
#include "valarray.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wreport/bulletin.h>
#include <cstdio>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct plan_shar
{
    plan_shar()
    {
    }

    ~plan_shar()
    {
    }
};
TESTGRP(plan);

static void read_nth_bufr(BufrBulletin& bulletin, const std::string& testname, unsigned idx=0)
{
    // Open input file
    string srcfile(b2nc::tests::datafile("bufr/" + testname));
    FILE* infd = fopen(srcfile.c_str(), "rb");
    if (infd == NULL)
        error_system::throwf("cannot open %s", srcfile.c_str());

    string rawmsg;
    for (unsigned i = 0; i <= idx && BufrBulletin::read(infd, rawmsg, srcfile.c_str()); ++i)
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);
    }

    fclose(infd);
}

template<> template<>
void to::test<1>()
{
    Options opts;

    BufrBulletin bulletin;
    read_nth_bufr(bulletin, "cdfin_acars");

    Plan plan;
    plan.build(bulletin, opts);

    // Check toplevel section
    const plan::Section* s = plan.sections[0];

    // Normal variable
    ensure(s->entries[0]->data);
    ensure_equals(s->entries[0]->data->name, "MMIOGC");

    // Delayed replication count goes in containing section
    ensure(s->entries[25]->data);
    ensure_equals(s->entries[25]->data->name, "MDREP");

    // Replicated sections are represented by a placeholder
    ensure(!s->entries[26]->data);
    ensure(s->entries[26]->subsection);
    ensure_equals(s->entries[26]->subsection->id, 1);

    // The containing section continues fine after the replicated section
    ensure(s->entries[27]->data);
    ensure_equals(s->entries[27]->data->name, "MTUIN");

    // Check replicated section
    s = plan.sections[1];

    // Normal variables are there, even if the section might be repeated 0
    // times
    ensure(s->entries[0]->data);
    ensure_equals(s->entries[0]->data->name, "MMTI");

    //plan.print(stderr);
}

}

// vim:set ts=4 sw=4:

