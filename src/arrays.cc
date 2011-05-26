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

#include "arrays.h"
#include "utils.h"
#include <wreport/var.h>
#include <wreport/bulletin.h>
#include <netcdf.h>

using namespace wreport;
using namespace std;

namespace b2nc {

struct SingleValArray : public ValArray
{
    std::vector<Var> vars;

    void add(const Var& var, unsigned nesting=0)
    {
        vars.push_back(Var(var, false));
    }

    const Var* get_var(unsigned nesting, unsigned pos) const
    {
        if (nesting > 0) return NULL;
        if (pos >= vars.size()) return NULL;
        return &vars[pos];
    }
    size_t get_size(unsigned nesting) const
    {
        if (nesting > 0) return 0;
        return vars.size();
    }

    int define(int ncid, int bufrdim) const
    {
        // Skip variable if it's never been found
        if (vars.empty())
            return -1;

        int dims[1] = { bufrdim };
        int resid;
        nc_type type;
        Varinfo info = vars[0].info();
        if (info->is_string())
            // TODO: add extra dim
            type = NC_CHAR;
        else if (info->scale == 0)
            type = NC_INT;
        else
            type = NC_FLOAT; // TODO: why not double?
        int res = nc_def_var(ncid, name.c_str(), type, 1, dims, &resid);
        error_netcdf::throwf_iferror(res, "creating variable %s", name.c_str());
        return resid;
    }

    void dump(FILE* out)
    {
        for (size_t i = 0; i < vars.size(); ++i)
        {
            string formatted = vars[i].format();
            fprintf(out, "%s[%zd]: %s\n", name.c_str(), i, formatted.c_str());
        }
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

    const Var* get_var(unsigned nesting, unsigned pos) const
    {
        if (nesting >= arrs.size()) return NULL;
        if (pos >= arrs[nesting].vars.size()) return NULL;
        return &arrs[nesting].vars[pos];
    }

    size_t get_size(unsigned nesting) const
    {
        if (nesting >= arrs.size()) return 0;
        return arrs[nesting].vars.size();
    }

    int define(int ncid, int bufrdim) const
    {
        // TODO
        return -1;
    }

    void dump(FILE* out)
    {
        for (size_t a = 0; a < arrs.size(); ++a)
            for (size_t i = 0; i < arrs[1].vars.size(); ++i)
            {
                string formatted = arrs[a].vars[i].format();
                fprintf(out, "%s[%zd,%zd]: %s\n", name.c_str(), a, i, formatted.c_str());
            }
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
    std::vector<unsigned> rep_stack;
    string tag;

    void update_tag()
    {
        tag.clear();
        for (unsigned i = 0; i < rep_nesting; ++i)
        {
            if (i != 0) tag += "_";
            char buf[20];
            snprintf(buf, 20, "%u", rep_stack[rep_nesting]);
            tag += buf;
        }
    }

public:
    ArrayBuilder(const Bulletin& bulletin, Arrays& arrays)
        : bulletin::ConstBaseDDSExecutor(bulletin), arrays(arrays), rep_nesting(0) {}

    virtual void start_subset(unsigned subset_no)
    {
        bulletin::ConstBaseDDSExecutor::start_subset(subset_no);
        if (rep_nesting != 0)
            error_consistency::throwf("At start of subset, rep_nesting is %u instead of 0", rep_nesting);
        rep_stack.clear();
        update_tag();
        arrays.start(tag);
    }

    virtual void push_repetition(unsigned length, unsigned count)
    {
        ++rep_nesting;
        while (rep_nesting >= rep_stack.size())
            rep_stack.push_back(0u);
        ++(rep_stack[rep_nesting]);
        update_tag();
    }

    virtual void start_repetition()
    {
        arrays.start(tag);
    }

    virtual void pop_repetition()
    {
        --rep_nesting;
        update_tag();
    }

    virtual void encode_var(Varinfo info, unsigned var_pos)
    {
        const Var& var = get_var(var_pos);
        ValArray& arr = arrays.get_valarray(Namer::DT_DATA, var, tag);
        arr.add(var, rep_nesting);

        // TODO: encode attributes
    }

    virtual void encode_char_data(Varcode code, unsigned var_pos)
    {
        //const Var& var = get_var(var_pos);
        // TODO
    }
};



Arrays::Arrays(Namer::Type type)
    : namer(Namer::get(type).release()) {}

Arrays::~Arrays()
{
    delete namer;
}

void Arrays::start(const std::string& tag)
{
    namer->start(tag);
}

ValArray& Arrays::get_valarray(const char* type, const Var& var, const std::string& tag)
{
    string name = namer->name(type, var.code(), tag);

    map<string, unsigned>::const_iterator i = byname.find(name);
    if (i != byname.end())
        // Reuse array
        return *arrays[i->second];

    // Create a new array
    auto_ptr<ValArray> arr;
    if (tag.empty())
        arr.reset(new SingleValArray);
    else
        arr.reset(new MultiValArray);
    arr->name = name;
    arrays.push_back(arr.release());
    byname[name] = arrays.size() - 1;
    return *arrays.back();
}

void Arrays::add(const Bulletin& bulletin)
{
    ArrayBuilder ab(bulletin, *this);
    bulletin.run_dds(ab);
}

void Arrays::dump(FILE* out)
{
    for (std::vector<ValArray*>::const_iterator i = arrays.begin();
            i != arrays.end(); ++i)
    {
        ValArray& va = **i;
        va.dump(out);
    }
}

}
