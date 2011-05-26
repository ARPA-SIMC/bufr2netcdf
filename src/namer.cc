/*
 * namers - Naming stategies for NetCDF data arrays
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

#include "namer.h"
#include <wreport/error.h>
#include <map>
#include <cstdio>

using namespace wreport;
using namespace std;

namespace b2nc {

/**
 * Type_FXXYYY_RRR style namer
 */
class PlainNamer : public Namer
{
protected:
    std::map<Varcode, int> seen_count;

    std::map<Varcode, int>::iterator get_seen_count(Varcode code)
    {
        std::map<Varcode, int>::iterator i = seen_count.find(code);
        if (i != seen_count.end()) return i;

        pair< std::map<Varcode, int>::iterator, bool > r = seen_count.insert(make_pair(code, 0));
        return r.first;
    }

public:

    virtual std::string name(Varcode code, bool nested=false)
    {
        const char* type = "TODO";
        string fcode = varcode_format(code);
        unsigned index;

        std::map<Varcode, int>::iterator i = get_seen_count(code);
        if (nested)
            index = i->second;
        else
            index = ++(i->second);

        char buf[20];
        snprintf(buf, 20, "%s_%c%02d%03d_%03d",
                type,
                "BRCD"[WR_VAR_F(code)],
                WR_VAR_X(code),
                WR_VAR_Y(code),
                index);
        return buf;
    }
};

auto_ptr<Namer> Namer::get(Type type)
{
    switch (type)
    {
        case PLAIN: return auto_ptr<Namer>(new PlainNamer);
        case MNEMONIC: throw error_unimplemented("mnemonic namers not yet implemented");
        default:
            error_consistency::throwf("requested namer for unsupported type %d", (int)type);
    }
}

}
