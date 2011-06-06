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

const char* Namer::ENV_TABLE_DIR = "B2NC_TABLES";
const char* Namer::DEFAULT_TABLE_DIR = "/usr/share/bufr2netcdf";

namespace {

static const char* type_names[] = {
    "Data",
    "QBits",
    "Char",
    "QAInfo",
   };

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

struct CounterSet : public std::map<string, Counter>
{
    SeenCounter seen_counter;

    Counter& get_counter(const std::string& tag)
    {
        iterator i = find(tag);
        if (i != end())
            return i->second;
        pair<iterator, bool> res = insert(
                make_pair(tag, Counter(seen_counter)));
        return res.first->second;
    }
};

/**
 * Type_FXXYYY_RRR style namer
 */
class PlainNamer : public Namer
{
protected:
    const mnemo::Table* table;
    CounterSet counters[DT_MAX];

public:
    PlainNamer()
    {
        // TODO: choose after first BUFR in the bunch?
        table = mnemo::Table::get(14);
    }

    virtual void start(const std::string& tag)
    {
        for (int i = 0; i < DT_MAX; ++i)
            counters[i].get_counter(tag).reset();
    }

    virtual unsigned name(DataType type, Varcode code, const std::string& tag, std::string& name, std::string& mnemo)
    {
        // Get/create counter for this tag
        Counter& counter = counters[type].get_counter(tag);
        Varcode effective_code = type == DT_CHAR ? WR_VAR(2, 8, 0) : code;
        unsigned index = counter.get_index(effective_code);

        // Plain name
        {
            char buf[20];
            snprintf(buf, 20, "%s_%c%02d%03d_%03d",
                    type_names[type],
                    "BRCD"[WR_VAR_F(code)],
                    WR_VAR_X(code),
                    WR_VAR_Y(code),
                    index);
            name = buf;
        }

        // Mnemonic name
        const char* mname = table->find(effective_code);
        if (mname)
        {
            mnemo = mname;

            if (index > 0)
            {
                char buf[20];
                snprintf(buf, 20, "%d", index-1);
                mnemo += buf;
            }

            switch (type)
            {
                case DT_QBITS: mnemo += "Q"; break;
                default: break;
            }
        } else
            mnemo.clear();

        return index;
    }
};

struct MnemoNamer : public PlainNamer
{
    virtual unsigned name(DataType type, Varcode code, const std::string& tag, std::string& name, std::string& mnemo)
    {
        unsigned index = PlainNamer::name(type, code, tag, name, mnemo);
        if (!mnemo.empty())
            name = mnemo;
        return index;
    }
};

}

const char* Namer::type_name(DataType type)
{
    return type_names[type];
}

auto_ptr<Namer> Namer::get(Type type)
{
    switch (type)
    {
        case PLAIN: return auto_ptr<Namer>(new PlainNamer);
        case MNEMONIC: return auto_ptr<Namer>(new MnemoNamer);
        default:
            error_consistency::throwf("requested namer for unsupported type %d", (int)type);
    }
}

}
