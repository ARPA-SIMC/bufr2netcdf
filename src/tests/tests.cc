/*
 * tests/tests - Unit test utilities
 *
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "tests/tests.h"
#include <wreport/error.h>
#include <wreport/utils/string.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>

using namespace wreport;
using namespace wreport::tests;
using namespace std;

namespace b2nc {
namespace tests {

std::string datafile(const std::string& fname)
{
    const char* testdatadirenv = getenv("B2NC_TESTDATA");
    std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
    return str::joinpath(testdatadir, fname);
}

std::string slurpfile(const std::string& name)
{
    string fname = datafile(name);
    string res;

    FILE* fd = fopen(fname.c_str(), "rb");
    if (fd == NULL)
        error_system::throwf("opening %s", fname.c_str());

    /* Read the entire file contents */
    while (!feof(fd))
    {
        char c;
        if (fread(&c, 1, 1, fd) == 1)
            res += c;
    }

    fclose(fd);

    return res;
}

LocalEnv::LocalEnv(const std::string& key, const std::string& val)
    : key(key)
{
    const char* v = getenv(key.c_str());
    oldVal = v == NULL ? "" : v;
    setenv(key.c_str(), val.c_str(), 1);
}

LocalEnv::~LocalEnv()
{
    setenv(key.c_str(), oldVal.c_str(), 1);
}

}
}
