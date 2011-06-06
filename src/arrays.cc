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
#include "ncoutfile.h"
#include <wreport/var.h>
#include <wreport/bulletin.h>
#include <netcdf.h>
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {

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
    ValArray* loop_var;

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
        : bulletin::ConstBaseDDSExecutor(bulletin), arrays(arrays), rep_nesting(0), bufr_idx(bufr_idx), loop_var(0) {}

    virtual void start_subset(unsigned subset_no)
    {
        bulletin::ConstBaseDDSExecutor::start_subset(subset_no);
        if (rep_nesting != 0)
            error_consistency::throwf("At start of subset, rep_nesting is %u instead of 0", rep_nesting);
        rep_stack.clear();
        update_tag();
        arrays.start(tag);
        ++bufr_idx;
        loop_var = 0;
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
        loop_var = 0;
    }

    virtual void encode_var(Varinfo info, unsigned var_pos)
    {
        const Var& var = get_var(var_pos);
        ValArray& arr = arrays.get_valarray(Namer::DT_DATA, var, tag);
        arr.add(var, bufr_idx);

        if (arr.newly_created)
        {
            arr.loop_var = loop_var;
            arr.newly_created = false;
        }

        if (const Var* a = var.enqa(WR_VAR(0, 33, 50)))
        {
            ValArray& attr_arr = arrays.get_valarray(Namer::DT_QBITS, var, tag, a, &arr);
            attr_arr.add(*a, bufr_idx);
            if (attr_arr.newly_created)
            {
                arr.loop_var = loop_var;
                attr_arr.newly_created = false;
            }
        }

        // Add references information to arr
        if (arr.references.empty())
            for (map<Varcode, string>::const_iterator i = context.begin();
                    i != context.end(); ++i)
                arr.references.push_back(*i);

        // Update current context information
        if (WR_VAR_X(var.code()) < 10)
            context[var.code()] = arr.name;

        if (WR_VAR_X(var.code()) == 31)
        {
            switch (var.code())
            {
                case WR_VAR(0, 31, 1):
                case WR_VAR(0, 31, 2):
                    loop_var = &arr;
                    break;
            }
        }
    }

    virtual void encode_char_data(Varcode code, unsigned var_pos)
    {
        const Var& var = get_var(var_pos);
        ValArray& arr = arrays.get_valarray(Namer::DT_CHAR, var, tag);
        arr.add(var, bufr_idx);
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

ValArray& Arrays::get_valarray(Namer::DataType type, const Var& var, const std::string& tag, const Var* attr, ValArray* master)
{
    string name, mnemo;
    unsigned rcnt = namer->name(type, var.code(), tag, name, mnemo);

    map<string, unsigned>::const_iterator i = byname.find(name);
    if (i != byname.end())
        // Reuse array
        return *arrays[i->second];

    // Use attr as the real info for the variable, defaulting to var.info()
    Varinfo info = attr ? attr->info() : var.info();

    // Create a new array
    auto_ptr<ValArray> arr;
    if (tag.empty())
        arr.reset(ValArray::make_singlevalarray(type, info));
    else
    {
        map<string, LoopInfo>::const_iterator i = dimnames.find(tag);
        if (i != dimnames.end())
            arr.reset(ValArray::make_multivalarray(type, info, i->second));
        else
        {
            char buf[20];
            snprintf(buf, 20, "Loop_%03d_maxlen", loop_idx);
            ++loop_idx;
            pair<std::map<std::string, LoopInfo>::iterator, bool> res =
                dimnames.insert(make_pair(tag, LoopInfo(buf, arrays.size())));
            arr.reset(ValArray::make_multivalarray(type, info, res.first->second));
        }
    }
    arr->name = name;
    arr->mnemo = mnemo;
    arr->rcnt = rcnt;
    arr->type = type;
    arr->master = master;
    if (master)
        master->slaves.push_back(arr.get());
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

bool Arrays::define(NCOutfile& outfile)
{
    int ncid = outfile.ncid;
    int bufrdim = outfile.dim_bufr_records;

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

void Arrays::putvar(NCOutfile& outfile) const
{
    int ncid = outfile.ncid;
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

bool Sections::define(NCOutfile& outfile)
{
    int ncid = outfile.ncid;

    if (max_length == 0)
        return false;

    char name[20];
    snprintf(name, 20, "section%d_length", idx);

    int res = nc_def_dim(ncid, name, max_length, &nc_dimid);
    error_netcdf::throwf_iferror(res, "creating %s dimension", name);

    snprintf(name, 20, "section%d", idx);

    int dims[] = { outfile.dim_bufr_records, nc_dimid };
    res = nc_def_var(ncid, name, NC_BYTE, 2, dims, &nc_varid);
    error_netcdf::throwf_iferror(res, "creating variable %s", name);

    return true;
}

void Sections::putvar(NCOutfile& outfile) const
{
    int ncid = outfile.ncid;

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

bool IntArray::define(NCOutfile& outfile)
{
    int ncid = outfile.ncid;
    int bufrdim = outfile.dim_bufr_records;

    if (values.empty()) return false;
    int res = nc_def_var(ncid, name.c_str(), NC_INT, 1, &bufrdim, &nc_varid);
    error_netcdf::throwf_iferror(res, "creating variable %s", name.c_str());

    int missing = NC_FILL_INT;
    res = nc_put_att_int(ncid, nc_varid, "_FillValue", NC_INT, 1, &missing);
    error_netcdf::throwf_iferror(res, "creating _FillValue attribute in %s", name.c_str());

    return true;
}

void IntArray::putvar(NCOutfile& outfile) const
{
    if (values.empty()) return;
    size_t start[] = {0};
    size_t count[] = {values.size()};
    int res = nc_put_vara_int(outfile.ncid, nc_varid, start, count, values.data());
    error_netcdf::throwf_iferror(res, "storing %zd integer values", values.size());
}

}
