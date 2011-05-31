/*
 * mnemo - Mnemonic variable names resolution
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

#include "mnemo.h"
#include <wreport/error.h>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {
namespace mnemo {

const char* ENV_TABLE_DIR = "B2NC_TABLES";
const char* DEFAULT_TABLE_DIR = "/usr/share/bufr2netcdf";

Record::Record() : code(0)
{
    name[0] = 0;
}

Record::Record(Varcode code, const char* name)
    : code(code)
{
    strncpy(this->name, name, 10);
    this->name[9] = 0;
}

bool Record::operator<(const Record& r) const
{
    return code < r.code;
}

Table::Table(int version)
    : version(version)
{
    load();
}

void Table::load()
{
    clear();

    char fname[10];
    snprintf(fname, 10, "mnem_%03d", version);
    const char* dir = getenv(ENV_TABLE_DIR);
    if (dir == NULL)
        dir = DEFAULT_TABLE_DIR;
    string pathname(dir);
    if (pathname[pathname.size() - 1] != '/')
        pathname += '/';
    pathname += fname;

    FILE* in = fopen(pathname.c_str(), "rt");
    if (in == NULL)
        error_system::throwf("opening file %s", pathname.c_str());
    char line[30];
    unsigned lineno = 1;
    while (fgets(line, 30, in))
    {
        int f, x, y;
        char name[10];
        if (sscanf(line, " %d %d %d %9s", &f, &x, &y, name) != 4)
            throw error_parse(pathname.c_str(), lineno, "line has an unknown format");
        push_back(Record(WR_VAR(f, x, y), name));
        ++lineno;
    }
    if (ferror(in))
        error_system::throwf("reading from file %s", pathname.c_str());
    fclose(in);

    std::sort(begin(), end());
}

const char* Table::find(Varcode code) const
{
    Record sample(code, "");
    const_iterator i = lower_bound(begin(), end(), sample);
    if (i == end() || i->code != code)
        return NULL;
    else
        return i->name;
}

const Table* Table::get(int version)
{
    static map<int, Table*> table_cache;

    map<int, Table*>::const_iterator i = table_cache.find(version);
    if (i != table_cache.end())
        return i->second;

    Table* res = new Table(version);
    table_cache[version] = res;
    return res;
}

}
}
