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
    BufrBulletin bulletin;
    read_nth_bufr(bulletin, "cdfin_acars");

    Plan plan;
    plan.build(bulletin);

    plan.print(stderr);

}

}

// vim:set ts=4 sw=4:

