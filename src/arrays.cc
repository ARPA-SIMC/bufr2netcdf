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
#include "mnemo.h"
#include <wreport/var.h>
#include <wreport/bulletin.h>
#include <netcdf.h>
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {

struct BaseValArray : public ValArray
{
    BaseValArray(Varinfo info) : ValArray(info) {}

    void add_common_attributes(int ncid)
    {
        int res;
        if (info->is_string())
        {
            char missing = NC_FILL_CHAR;
            res = nc_put_att_text(ncid, nc_varid, "_FillValue", 1, &missing);
        }
        else if (info->scale == 0)
        {
            int missing = NC_FILL_INT;
            res = nc_put_att_int(ncid, nc_varid, "_FillValue", NC_INT, 1, &missing);
        }
        else
        {
            float missing = NC_FILL_FLOAT;
            res = nc_put_att_float(ncid, nc_varid, "_FillValue", NC_FLOAT, 1, &missing);
        }
        error_netcdf::throwf_iferror(res, "setting _FillValue attribute for %s", name.c_str());

        res = nc_put_att_text(ncid, nc_varid, "long_name", strlen(info->desc), info->desc);
        error_netcdf::throwf_iferror(res, "setting long_name attribute for %s", name.c_str());

        res = nc_put_att_text(ncid, nc_varid, "units", strlen(info->unit), info->unit);
        error_netcdf::throwf_iferror(res, "setting unit attribute for %s", name.c_str());

        res = nc_put_att_text(ncid, nc_varid, "type", strlen("Data"), "Data"); // TODO
        error_netcdf::throwf_iferror(res, "setting type attribute for %s", name.c_str());

        {
            int ifxy = WR_VAR_F(info->var) * 100000 + WR_VAR_X(info->var) * 1000 + WR_VAR_Y(info->var);
            res = nc_put_att_int(ncid, nc_varid, "ifxy", NC_INT, 1, &ifxy);
            error_netcdf::throwf_iferror(res, "setting ifxy attribute for %s", name.c_str());
        }

        {
            int rcnt = this->rcnt;
            res = nc_put_att_int(ncid, nc_varid, "rcnt", NC_INT, 1, &rcnt);
            error_netcdf::throwf_iferror(res, "setting rcnt attribute for %s", name.c_str());
        }

        res = nc_put_att_text(ncid, nc_varid, "dim0_length", strlen("_constant"), "_constant"); // TODO
        error_netcdf::throwf_iferror(res, "setting dim0_length attribute for %s", name.c_str());

        res = nc_put_att_text(ncid, nc_varid, "mnemonic", mnemo.size(), mnemo.data());
        error_netcdf::throwf_iferror(res, "setting mnemonic attribute for %s", name.c_str());

        // Refrences attributes
        if (!references.empty())
        {
            int ref_codes[references.size()];
            for (size_t i = 0; i < references.size(); ++i)
            {
                Varcode code = references[i].first;
                const std::string& name = references[i].second;

                ref_codes[i] = WR_VAR_F(code) * 100000
                             + WR_VAR_X(code) * 1000
                             + WR_VAR_Y(code);

                char att_name[20];
                snprintf(att_name, 20, "reference_%01d%02d%03d",
                        WR_VAR_F(code),
                        WR_VAR_X(code),
                        WR_VAR_Y(code));

                res = nc_put_att_text(ncid, nc_varid, att_name, name.size(), name.data());
                error_netcdf::throwf_iferror(res, "setting %s attribute for %s", att_name, this->name.c_str());
            }

            res = nc_put_att_int(ncid, nc_varid, "references", NC_INT, references.size(), ref_codes);
            error_netcdf::throwf_iferror(res, "setting references attribute for %s", name.c_str());
        }

    }
};

struct SingleValArray : public BaseValArray
{
    std::vector<Var> vars;

    SingleValArray(Varinfo info) : BaseValArray(info) {}

    void add(const Var& var, unsigned bufr_idx=0)
    {
        while (vars.size() < bufr_idx)
            vars.push_back(Var(var.info()));
        if (vars.size() == bufr_idx)
            vars.push_back(Var(var, false));
    }

    Var get_var(unsigned bufr_idx, unsigned rep) const
    {
        if (rep > 0) return Var(info);
        if (bufr_idx >= vars.size()) return Var(info);
        return vars[bufr_idx];
    }

    size_t get_size() const
    {
        return vars.size();
    }

    size_t get_max_rep() const
    {
        return 1;
    }

    bool define(int ncid, int bufrdim)
    {
        // Skip variable if it's never been found
        if (vars.empty())
        {
            nc_varid = -1;
            return false;
        }

        int dims[2] = { bufrdim, 0 };
        int ndims = 1;
        nc_type type;
        Varinfo info = vars[0].info();
        if (info->is_string())
        {
            string dimname = name + "_strlen";
            int res = nc_def_dim(ncid, dimname.c_str(), info->len, &dims[1]);
            error_netcdf::throwf_iferror(res, "creating %s dimension", dimname.c_str());
            ++ndims;
            type = NC_CHAR;
        }
        else if (info->scale == 0)
            type = NC_INT;
        else
            type = NC_FLOAT; // TODO: why not double?

        int res = nc_def_var(ncid, name.c_str(), type, ndims, dims, &nc_varid);
        error_netcdf::throwf_iferror(res, "creating variable %s", name.c_str());

        add_common_attributes(ncid);

        return nc_varid;
    }

    void putvar(int ncid) const
    {
        if (vars.empty()) return;

        Varinfo info = vars[0].info();
        if (info->is_string())
        {
            size_t start[] = {0, 0};
            size_t count[] = {1, info->len};
            char missing[info->len]; // Missing value
            memset(missing, NC_FILL_CHAR, info->len);
            char value[info->len]; // Space-padded value
            for (size_t i = 0; i < vars.size(); ++i)
            {
                int res;
                start[0] = i;
                if (vars[i].isset())
                {
                    size_t len = strlen(vars[i].value());
                    memcpy(value, vars[i].value(), len);
                    for (size_t i = len; i < info->len; ++i)
                        value[i] = ' ';
                    res = nc_put_vara_text(ncid, nc_varid, start, count, value);
                } else
                    res = nc_put_vara_text(ncid, nc_varid, start, count, missing);
                error_netcdf::throwf_iferror(res, "storing %zd string values", vars.size());
            }
        }
        else if (info->scale == 0)
        {
            size_t start[] = {0};
            size_t count[] = {vars.size()};
            int vals[vars.size()];
            for (size_t i = 0; i < vars.size(); ++i)
            {
                if (vars[i].isset())
                    vals[i] = vars[i].enqi();
                else
                    vals[i] = NC_FILL_INT;
            }
            int res = nc_put_vara_int(ncid, nc_varid, start, count, vals);
            error_netcdf::throwf_iferror(res, "storing %zd integer values", vars.size());
        }
        else
        {
            size_t start[] = {0};
            size_t count[] = {vars.size()};
            float vals[vars.size()];
            for (size_t i = 0; i < vars.size(); ++i)
            {
                if (vars[i].isset())
                    vals[i] = vars[i].enqd();
                else
                    vals[i] = NC_FILL_FLOAT;
            }
            int res = nc_put_vara_float(ncid, nc_varid, start, count, vals);
            error_netcdf::throwf_iferror(res, "storing %zd float values", vars.size());
        }
    }

    void dump(FILE* out)
    {
        for (size_t i = 0; i < vars.size(); ++i)
        {
            string formatted = vars[i].format();
            fprintf(out, "%s[%zd]: %s\n", name.c_str(), i, formatted.c_str());
        }
    }

    void fill_int(int* vec, size_t size) const
    {
        for (unsigned i = 0; i < size; ++i)
        {
            if (i >= vars.size() || !vars[i].isset())
                vec[i] = NC_FILL_INT;
            else
                vec[i] = vars[i].enqi();
        }
    }

    void fill_float(float* vec, size_t size) const
    {
        for (unsigned i = 0; i < size; ++i)
        {
            if (i >= vars.size() || !vars[i].isset())
                vec[i] = NC_FILL_FLOAT;
            else
                vec[i] = vars[i].enqd();
        }
    }
};

struct MultiValArray : public BaseValArray
{
    std::vector<SingleValArray> arrs;
    const Arrays::LoopInfo& loopinfo;

    MultiValArray(Varinfo info, const Arrays::LoopInfo& loopinfo) : BaseValArray(info), loopinfo(loopinfo) {}

    void add(const wreport::Var& var, unsigned bufr_idx)
    {
        // Ensure we have the right number of dimensions
        while (bufr_idx >= arrs.size())
            arrs.push_back(SingleValArray(info));

        arrs[bufr_idx].add(var, arrs[bufr_idx].get_size());
    }

    Var get_var(unsigned bufr_idx, unsigned rep=0) const
    {
        if (bufr_idx >= arrs.size()) return Var(info);
        if (rep >= arrs[bufr_idx].vars.size()) return Var(info);
        return arrs[bufr_idx].vars[rep];
    }

    size_t get_size() const
    {
        return arrs.size();
    }

    size_t get_max_rep() const
    {
        size_t res = 0;
        for (std::vector<SingleValArray>::const_iterator i = arrs.begin();
                i != arrs.end(); ++i)
        {
            size_t s = i->get_size();
            if (s > res) res = s;
        }
        return res;
    }

    bool define(int ncid, int bufrdim)
    {
        // Skip variable if it's never been found
        if (arrs.empty())
        {
            nc_varid = -1;
            return false;
        }

        int dims[3] = { bufrdim, loopinfo.nc_dimid, 0 };
        int ndims = 2;
        nc_type type;
        if (info->is_string())
        {
            string dimname = name + "_strlen";
            int res = nc_def_dim(ncid, dimname.c_str(), info->len, &dims[2]);
            error_netcdf::throwf_iferror(res, "creating %s dimension", dimname.c_str());
            ++ndims;
            type = NC_CHAR;
        }
        else if (info->scale == 0)
            type = NC_INT;
        else
            type = NC_FLOAT; // TODO: why not double?

        int res = nc_def_var(ncid, name.c_str(), type, ndims, dims, &nc_varid);
        error_netcdf::throwf_iferror(res, "creating variable %s", name.c_str());

        add_common_attributes(ncid);

        res = nc_put_att_text(ncid, nc_varid, "dim1_length", strlen("_constant"), "_constant"); // TODO
        error_netcdf::throwf_iferror(res, "setting dim1_length attribute for %s", name.c_str());

        return true;
    }

    void putvar(int ncid) const
    {
        if (arrs.empty()) return;

        size_t arrsize = get_max_rep();
        size_t start[] = {0, 0, 0};
        size_t count[] = {1, arrsize, 0};

        if (info->is_string())
        {
            count[2] = info->len;
            char missing[info->len]; // Missing value
            memset(missing, NC_FILL_CHAR, info->len);
            char value[info->len]; // Space-padded value
            for (size_t i = 0; i < arrs.size(); ++i)
            {
                const SingleValArray& va = arrs[i];
                start[0] = i;
                for (size_t j = 0; j < arrsize; ++j)
                {
                    int res;
                    if (va.vars.size() > j && va.vars[j].isset())
                    {
                        size_t len = strlen(va.vars[j].value());
                        memcpy(value, va.vars[j].value(), len);
                        for (size_t i = len; i < info->len; ++i)
                            value[i] = ' ';
                        res = nc_put_vara_text(ncid, nc_varid, start, count, value);
                    } else
                        res = nc_put_vara_text(ncid, nc_varid, start, count, missing);
                    error_netcdf::throwf_iferror(res, "storing %zd string values", arrsize);
                }
            }
        }
        else if (info->scale == 0)
        {
            int vals[arrsize];
            for (unsigned i = 0; i < arrs.size(); ++i)
            {
                const SingleValArray& va = arrs[i];
                va.fill_int(vals, arrsize);
                start[0] = i;
                int res = nc_put_vara_int(ncid, nc_varid, start, count, vals);
                error_netcdf::throwf_iferror(res, "storing %zd integer values", arrsize);
            }
        }
        else
        {
            float vals[arrsize];
            for (unsigned i = 0; i < arrs.size(); ++i)
            {
                const SingleValArray& va = arrs[i];
                va.fill_float(vals, arrsize);
                start[0] = i;
                int res = nc_put_vara_float(ncid, nc_varid, start, count, vals);
                error_netcdf::throwf_iferror(res, "storing %zd float values", arrsize);
            }
        }
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
        encode_var(info, var_pos);
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
    map<Varcode, string> context;
    string tag;
    int& bufr_idx;

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
    ArrayBuilder(const Bulletin& bulletin, Arrays& arrays, int& bufr_idx)
        : bulletin::ConstBaseDDSExecutor(bulletin), arrays(arrays), rep_nesting(0), bufr_idx(bufr_idx) {}

    virtual void start_subset(unsigned subset_no)
    {
        bulletin::ConstBaseDDSExecutor::start_subset(subset_no);
        if (rep_nesting != 0)
            error_consistency::throwf("At start of subset, rep_nesting is %u instead of 0", rep_nesting);
        rep_stack.clear();
        update_tag();
        arrays.start(tag);
        ++bufr_idx;
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
        arr.add(var, bufr_idx);

        // Add references information to arr
        if (arr.references.empty())
            for (map<Varcode, string>::const_iterator i = context.begin();
                    i != context.end(); ++i)
                arr.references.push_back(*i);

        // Update current context information
        if (WR_VAR_X(var.code()) < 10)
            switch (var.code())
            {
                case WR_VAR(0, 1, 23):
                case WR_VAR(0, 1, 33):
                case WR_VAR(0, 8,  9):
                    // TODO: The test cases don't encode these: we skip them
                    // until we work out why they don't do it
                    break;
                default:
                    context[var.code()] = arr.name;
                    break;
            }
    }

    virtual void encode_char_data(Varcode code, unsigned var_pos)
    {
        //const Var& var = get_var(var_pos);
        // TODO
    }
};



Arrays::Arrays(Namer::Type type)
    : namer(Namer::get(type).release()),
      date_year(0), date_month(0), date_day(0),
      time_hour(0), time_minute(0), time_second(0),
      date_varid(-1), time_varid(-1),
      loop_idx(0), bufr_idx(-1)
{
}

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
    string name, mnemo;
    unsigned rcnt = namer->name(type, var.code(), tag, name, mnemo);

    map<string, unsigned>::const_iterator i = byname.find(name);
    if (i != byname.end())
        // Reuse array
        return *arrays[i->second];

    // Create a new array
    auto_ptr<ValArray> arr;
    if (tag.empty())
        arr.reset(new SingleValArray(var.info()));
    else
    {
        map<string, LoopInfo>::const_iterator i = dimnames.find(tag);
        if (i != dimnames.end())
            arr.reset(new MultiValArray(var.info(), i->second));
        else
        {
            char buf[20];
            snprintf(buf, 20, "Loop_%03d_maxlen", loop_idx);
            ++loop_idx;
            pair<std::map<std::string, LoopInfo>::iterator, bool> res =
                dimnames.insert(make_pair(tag, LoopInfo(buf, arrays.size())));
            arr.reset(new MultiValArray(var.info(), res.first->second));
        }
    }
    arr->name = name;
    arr->mnemo = mnemo;
    arr->rcnt = rcnt;
    arrays.push_back(arr.release());
    byname[name] = arrays.size() - 1;

    // Take note of significant ValArrays
    switch (var.code())
    {
        case WR_VAR(0, 4, 1): if (!date_year) date_year = arrays.back(); break;
        case WR_VAR(0, 4, 2): if (!date_month) date_month = arrays.back(); break;
        case WR_VAR(0, 4, 3): if (!date_day) date_day = arrays.back(); break;
        case WR_VAR(0, 4, 4): if (!time_hour) time_hour = arrays.back(); break;
        case WR_VAR(0, 4, 5): if (!time_minute) time_minute = arrays.back(); break;
        case WR_VAR(0, 4, 6): if (!time_second) time_second = arrays.back(); break;
    }

    return *arrays.back();
}

void Arrays::add(const Bulletin& bulletin)
{
    ArrayBuilder ab(bulletin, *this, bufr_idx);
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

bool Arrays::define(int ncid, int bufrdim)
{
    // Define the array size dimensions
    for (std::map<std::string, LoopInfo>::iterator i = dimnames.begin();
            i != dimnames.end(); ++i)
    {
        ValArray* va = arrays[i->second.firstarr];
        int res = nc_def_dim(ncid, i->second.dimname.c_str(), va->get_max_rep(), &i->second.nc_dimid);
        error_netcdf::throwf_iferror(res, "creating %s dimension", i->second.dimname.c_str());
    }

    for (vector<ValArray*>::const_iterator i = arrays.begin();
            i != arrays.end(); ++i)
        (*i)->define(ncid, bufrdim);

    if (date_year && date_month && date_day)
    {
        int res = nc_def_var(ncid, "DATE", NC_INT, 1, &bufrdim, &date_varid);
        error_netcdf::throwf_iferror(res, "creating variable DATE");

        int missing = NC_FILL_INT;
        res = nc_put_att_int(ncid, date_varid, "_FillValue", NC_INT, 1, &missing);
        error_netcdf::throwf_iferror(res, "creating _FillValue attribute in DATE");

        const char* attname = "Date as YYYYMMDD";
        res = nc_put_att_text(ncid, date_varid, "long_name", strlen(attname), attname);
        error_netcdf::throwf_iferror(res, "setting long_name attribute for DATE");

        res = nc_put_att_text(ncid, date_varid, "var_year", date_year->name.size(), date_year->name.data());
        error_netcdf::throwf_iferror(res, "setting var_year attribute for DATE");

        res = nc_put_att_text(ncid, date_varid, "var_month", date_month->name.size(), date_month->name.data());
        error_netcdf::throwf_iferror(res, "setting var_month attribute for DATE");

        res = nc_put_att_text(ncid, date_varid, "var_day", date_day->name.size(), date_day->name.data());
        error_netcdf::throwf_iferror(res, "setting var_day attribute for DATE");
    }

    if (time_hour)
    {
        int res = nc_def_var(ncid, "TIME", NC_INT, 1, &bufrdim, &time_varid);
        error_netcdf::throwf_iferror(res, "creating variable TIME");

        int missing = NC_FILL_INT;
        res = nc_put_att_int(ncid, time_varid, "_FillValue", NC_INT, 1, &missing);
        error_netcdf::throwf_iferror(res, "creating _FillValue attribute in TIME");

        const char* attname = "Time as HHMMSS";
        res = nc_put_att_text(ncid, time_varid, "long_name", strlen(attname), attname);
        error_netcdf::throwf_iferror(res, "setting long_name attribute for TIME");

        res = nc_put_att_text(ncid, time_varid, "var_hour", time_hour->name.size(), time_hour->name.data());
        error_netcdf::throwf_iferror(res, "setting time_hour attribute for TIME");

        if (time_minute)
        {
            res = nc_put_att_text(ncid, time_varid, "var_minute", time_minute->name.size(), time_minute->name.data());
            error_netcdf::throwf_iferror(res, "setting time_minute attribute for TIME");
        }

        if (time_second)
        {
            res = nc_put_att_text(ncid, time_varid, "var_second", time_second->name.size(), time_second->name.data());
            error_netcdf::throwf_iferror(res, "setting time_second attribute for TIME");
        }
    }
    return true;
}

void Arrays::putvar(int ncid) const
{
    for (vector<ValArray*>::const_iterator i = arrays.begin();
            i != arrays.end(); ++i)
        (*i)->putvar(ncid);

    if (date_year && date_month && date_day)
    {
        size_t size = date_year->get_size();
        int values[size];
        for (size_t i = 0; i < size; ++i)
        {
            Var vy = date_year->get_var(i, 0);
            Var vm = date_month->get_var(i, 0);
            Var vd = date_day->get_var(i, 0);
            if (vy.isset() && vm.isset() && vd.isset())
                values[i] = vy.enqi() * 10000 + vm.enqi() * 100 + vd.enqi();
            else
                values[i] = NC_FILL_INT;
        }

        size_t start[] = {0};
        size_t count[] = {size};
        int res = nc_put_vara_int(ncid, date_varid, start, count, values);
        error_netcdf::throwf_iferror(res, "storing %zd integer values in DATE variable", size);
    }

    if (time_hour)
    {
        size_t size = time_hour->get_size();
        int values[size];
        for (size_t i = 0; i < size; ++i)
        {
            Var th = time_hour->get_var(i, 0);
            if (th.isset())
            {
                values[i] = th.enqi() * 10000;
                if (time_minute)
                {
                    Var tm = time_minute->get_var(i, 0);
                    if (tm.isset()) values[i] += tm.enqi() * 100;
                }
                if (time_second)
                {
                   Var ts = time_second->get_var(i, 0);
                   if (ts.isset()) values[i] += ts.enqi();
                }
            }
            else
                values[i] = NC_FILL_INT;
        }

        size_t start[] = {0};
        size_t count[] = {size};
        int res = nc_put_vara_int(ncid, time_varid, start, count, values);
        error_netcdf::throwf_iferror(res, "storing %zd integer values in TIME variable", size);
    }
}

Sections::Sections(unsigned idx)
    : max_length(0), idx(idx), nc_dimid(-1), nc_varid(-1)
{
}

void Sections::add(const wreport::BufrBulletin& bulletin)
{
    if (!bulletin.raw_details)
    {
        values.push_back(string());
        return;
    }

    unsigned len = bulletin.raw_details->sec[idx+1] - bulletin.raw_details->sec[idx];
    if (len == 0)
    {
        values.push_back(string());
        return;
    }

    if (len > max_length)
        max_length = len;
    values.push_back(string((const char*)bulletin.raw_details->sec[idx], len));
}

bool Sections::define(int ncid, int bufrdim)
{
    if (max_length == 0)
        return false;

    char name[20];
    snprintf(name, 20, "section%d_length", idx);

    int res = nc_def_dim(ncid, name, max_length, &nc_dimid);
    error_netcdf::throwf_iferror(res, "creating %s dimension", name);

    snprintf(name, 20, "section%d", idx);

    int dims[] = { bufrdim, nc_dimid };
    res = nc_def_var(ncid, name, NC_BYTE, 2, dims, &nc_varid);
    error_netcdf::throwf_iferror(res, "creating variable %s", name);

    return true;
}

void Sections::putvar(int ncid) const
{
    if (max_length == 0)
        return;

    size_t start[] = {0, 0};
    size_t count[] = {1, 0};
    unsigned char missing[max_length]; // Missing value
    memset(missing, NC_FILL_BYTE, max_length);
    for (size_t i = 0; i < values.size(); ++i)
    {
        int res;
        start[0] = i;
        if (!values[i].empty())
        {
            count[1] = values[i].size();
            res = nc_put_vara_uchar(ncid, nc_varid, start, count, (const unsigned char*)values[i].data());
        }
        else
        {
            count[1] = max_length;
            res = nc_put_vara_uchar(ncid, nc_varid, start, count, missing);
        }
        error_netcdf::throwf_iferror(res, "storing %zd string values", count[1]);
    }
}

IntArray::IntArray(const std::string& name)
    : name(name), nc_varid(-1)
{
}

void IntArray::add(int val)
{
    values.push_back(val);
}
void IntArray::add_missing()
{
    values.push_back(NC_FILL_INT);
}

bool IntArray::define(int ncid, int bufrdim)
{
    if (values.empty()) return false;
    int res = nc_def_var(ncid, name.c_str(), NC_INT, 1, &bufrdim, &nc_varid);
    error_netcdf::throwf_iferror(res, "creating variable %s", name.c_str());

    int missing = NC_FILL_INT;
    res = nc_put_att_int(ncid, nc_varid, "_FillValue", NC_INT, 1, &missing);
    error_netcdf::throwf_iferror(res, "creating _FillValue attribute in %s", name.c_str());

    return true;
}

void IntArray::putvar(int ncid) const
{
    if (values.empty()) return;
    size_t start[] = {0};
    size_t count[] = {values.size()};
    int res = nc_put_vara_int(ncid, nc_varid, start, count, values.data());
    error_netcdf::throwf_iferror(res, "storing %zd integer values", values.size());
}

}
