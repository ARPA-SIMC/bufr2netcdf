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
#include "options.h"
#include "utils.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wreport/utils/sys.h>
#include <netcdf.h>
#include <cstdlib>

using namespace b2nc;
using namespace wreport;
using namespace wreport::tests;
using namespace std;

namespace {

static const char* testfname = "test-ncoutfile.nc";

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("unopened", []() {
            // Test that ncid is -1 when the file is not open
            Options opts;
            NCOutfile out(opts);

            wassert(actual(out.ncid) == -1);
            out.open(testfname);
            wassert(actual(out.ncid) != -1);
            out.close();
            wassert(actual(out.ncid) == -1);
        });

        add_method("creation", []() {
            // Test that the file is created if missing
            Options opts;
            NCOutfile out(opts);

            sys::unlink_ifexists(testfname);

            out.open(testfname);
            out.close();

            wassert(actual(sys::exists(testfname)).istrue());
        });
    }
} tests("ncoutfile");

}
