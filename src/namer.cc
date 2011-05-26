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
#include <vector>
#include <cstdio>

using namespace wreport;
using namespace std;

namespace b2nc {

const char* Namer::DT_DATA = "Data";
const char* Namer::DT_QBITS = "QBits";
const char* Namer::DT_CHAR = "Char";
const char* Namer::DT_QAINFO = "QAInfo";

namespace {

struct SeenCounter
{
    std::map<Varcode, unsigned> seen_count;

    unsigned allocate(Varcode code)
    {
        std::map<Varcode, unsigned>::iterator i = seen_count.find(code);
        if (i != seen_count.end())
            return ++(i->second);
        else
        {
            seen_count.insert(make_pair(code, 0));
            return 0;
        }
    }
};

struct Counter
{
    SeenCounter& seen_counter;
    std::vector< pair<Varcode, unsigned> > assigned;
    unsigned pos;

    Counter(SeenCounter& seen_counter) : seen_counter(seen_counter), pos(0) {}

    void reset() { pos = 0; }

    unsigned get_index(Varcode code)
    {
        unsigned index;
        if (pos >= assigned.size())
        {
            // Allocate a new index
            index = seen_counter.allocate(code);
            assigned.push_back(make_pair(code, index));
        } else {
            // Reuse old index
            const pair<Varcode, unsigned>& res = assigned[pos];
            if (code != res.first)
                // TODO: if this is how we report it, properly format varcodes
                error_consistency::throwf("BUFR structure differs: expected %d found %d", res.first, code);
            index = res.second;
        }
        ++pos;
        return index;
    }
};

/**
 * Type_FXXYYY_RRR style namer
 */
class PlainNamer : public Namer
{
protected:
    SeenCounter seen_counter;
    std::map<string, Counter> counters;

    Counter& get_counter(const std::string& tag)
    {
        std::map<string, Counter>::iterator i = counters.find(tag);
        if (i != counters.end())
            return i->second;
        pair<std::map<string, Counter>::iterator, bool> res = counters.insert(
                make_pair(tag, Counter(seen_counter)));
        return res.first->second;
    }

public:
    virtual void start(const std::string& tag)
    {
        get_counter(tag).reset();
    }

    virtual std::string name(const char* type, Varcode code, const std::string& tag)
    {
        string fcode = varcode_format(code);

        // Get/create counter for this tag
        Counter& counter = get_counter(tag);
        unsigned index = counter.get_index(code);

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

}


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
