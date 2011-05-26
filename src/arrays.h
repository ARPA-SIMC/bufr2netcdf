/*
 * arrays - Aggregate multiple BUFR contents into data arrays
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

#ifndef B2NC_ARRAYS_H
#define B2NC_ARRAYS_H

#include "namer.h"
#include <string>
#include <vector>
#include <map>
#include <cstdio>

namespace wreport {
struct Var;
struct Bulletin;
}

namespace b2nc {

struct ValArray
{
    std::string name;

    virtual ~ValArray() {}
    virtual void add(const wreport::Var& var, unsigned nesting=0) = 0;
    virtual void dump(FILE* out) = 0;
};

struct Arrays
{
    Namer* namer;
    std::vector<ValArray*> arrays;
    std::map<std::string, unsigned> byname;

    Arrays(Namer::Type type = Namer::PLAIN);
    ~Arrays();

    void start(const std::string& tag);
    ValArray& get_valarray(const wreport::Var& var, const std::string& tag);

    void add(const wreport::Bulletin& bulletin);

    void dump(FILE* out);
};

}

#endif

