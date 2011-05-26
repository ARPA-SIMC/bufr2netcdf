/*
 * convert - Convert a stream of BUFR messages to one single NetCDF file
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

#include "convert.h"
#include "namer.h"
#include <wreport/bulletin.h>
#include <map>

using namespace wreport;
using namespace std;

namespace b2nc {

struct ValArray
{
    string name;

    virtual ~ValArray() {}
    virtual void add(const Var& var, unsigned nesting=0) = 0;
};

struct SingleValArray : public ValArray
{
    std::vector<Var> vars;

    void add(const Var& var, unsigned nesting=0)
    {
        vars.push_back(Var(var, false));
    }
};

struct MultiValArray : public ValArray
{
    std::vector<SingleValArray> arrs;

    void add(const Var& var, unsigned nesting=0)
    {
        // Ensure we have the right number of dimensions
        while (nesting >= arrs.size())
            arrs.push_back(SingleValArray());

        arrs[nesting].add(var);
    }
};

struct Arrays
{
    Namer* namer;
    vector<ValArray*> arrays;
    map<string, unsigned> byname;

    Arrays(Namer::Type type = Namer::PLAIN)
        : namer(Namer::get(type)) {}

    ~Arrays()
    {
        delete namer;
    }

    ValArray& get_valarray(const Var& var, unsigned nesting = 0)
    {
        string name = namer->name(var.code(), nesting > 0);

        map<string, unsigned>::const_iterator i = byname.find(name);
        if (i != byname.end())
            // Reuse array
            return *arrays[i->second];

        // Create a new array
        auto_ptr<ValArray> arr;
        if (nesting)
            arr.reset(new MultiValArray);
        else
            arr.reset(new SingleValArray);
        arr->name = name;
        arrays.push_back(arr.release());
        byname[name] = arrays.size() - 1;
        return *arrays.back();
    }

};

class ArrayBuilder : public bulletin::ConstBaseDDSExecutor
{
protected:
    virtual void encode_attr(Varinfo info, unsigned var_pos, Varcode attr_code) {}
    virtual unsigned encode_repetition_count(Varinfo info, unsigned var_pos)
    {
        const Var& var = get_var(var_pos);
        return var.enqi();

    }
    virtual unsigned encode_bitmap_repetition_count(Varinfo info, const Var& bitmap)
    {
        return bitmap.info()->len;
    }
    virtual void encode_bitmap(const Var& bitmap) {}

    Arrays& arrays;
    unsigned rep_nesting;

public:
    ArrayBuilder(const Bulletin& bulletin, Arrays& arrays)
        : bulletin::ConstBaseDDSExecutor(bulletin), arrays(arrays), rep_nesting(0) {}

    virtual void start_subset(unsigned subset_no)
    {
        bulletin::ConstBaseDDSExecutor::start_subset(subset_no);
        if (rep_nesting != 0)
            error_consistency::throwf("At start of subset, rep_nesting is %u instead of 0", rep_nesting);
    }

    virtual void push_repetition(unsigned length, unsigned count)
    {
        ++rep_nesting;
    }

    virtual void pop_repetition()
    {
        --rep_nesting;
    }

    virtual void encode_var(Varinfo info, unsigned var_pos)
    {
        const Var& var = get_var(var_pos);
        ValArray& arr = arrays.get_valarray(var);
        arr.add(var, rep_nesting);

        // TODO: encode attributes
    }

    virtual void encode_char_data(Varcode code, unsigned var_pos)
    {
        //const Var& var = get_var(var_pos);
        // TODO
    }
};

void Converter::convert(FILE* in, int outncid)
{
    string rawmsg;
    BufrBulletin bulletin;

    Arrays arrays;
    while (BufrBulletin::read(in, rawmsg /* , fname = 0 */))
    {
        ArrayBuilder ab(bulletin, arrays);
        // Decode the BUFR message
        bulletin.decode(rawmsg);
        // TODO: if first, build metadata
        // Add contents to the various data arrays
        bulletin.run_dds(ab);
    }
    // TODO: add arrays to NetCDF
}

}
