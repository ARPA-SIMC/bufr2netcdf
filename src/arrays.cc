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
#include "options.h"
#include "config.h"
#include <wreport/var.h>
#include <wreport/bulletin.h>
//#include <wreport/bulletin/buffers.h>
#include <wreport/bulletin/internals.h>
#include <wreport/utils/sys.h>
#include <netcdf.h>
#include <stack>
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {

/**
 * wreport encoder/interpreter which sends bulletin data to an Arrays object
 */
class ArrayBuilder : public bulletin::UncompressedEncoder
{
protected:
    //virtual void do_attr(Varinfo info, unsigned var_pos, Varcode attr_code) {}
    //virtual void do_associated_field(unsigned bit_count, unsigned significance) {}
    //virtual const Var& do_semantic_var(Varinfo info)
    //{
    //    const Var& var = get_var(current_var);
    //    convert_var(info);
    //    return var;
    //}

    Arrays& arrays;

    std::stack<plan::Section*> cur_section;

    map<Varcode, const ValArray*> context;
    unsigned bufr_idx;

    /*
     * The next two variables are used to deal with [begin,end] hour ranges
     * being represented by two consecutive B04024. They implement a
     * generalisation of the hack.
     */

    // Previously seen varcode (0 at start of subset or if the previous varcode
    // had xx >= 10)
    Varcode prev_code;
    // Number of times in a row the previous varcode appeared
    unsigned prev_code_count;

    plan::Variable& variable_for(Varcode code)
    {
        plan::Variable& v = cur_section.top()->current();
        if (v.subsection)
        {
            if (arrays.verbose)
            {
                fprintf(stderr, "Trying to match %01d%02d%03d with ",
                        WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
                v.print(stderr);
            }
            error_consistency::throwf("out of sync at %u: value is a subsection instead of a variable", cur_section.top()->cursor);
        } else {
            if (arrays.debug)
            {
                fprintf(stderr, "Matched %01d%02d%03d with ",
                        WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
                v.print(stderr);
            }
        }
        if (v.data && v.data->info->code != code)
            error_consistency::throwf("out of sync at %u: vars mismatch (%d%02d%03d != %d%02d%03d)",
                    cur_section.top()->cursor, WR_VAR_FXY(v.data->info->code), WR_VAR_FXY(code));
        return v;
    }

    plan::Section* section_for_current_position()
    {
        plan::Section* plan_sec = cur_section.top()->current().subsection;
        if (!plan_sec)
        {
            if (arrays.verbose)
            {
                fprintf(stderr, "Looking for section, got ");
                cur_section.top()->current().print(stderr);
            }
            error_consistency::throwf("out of sync at %u: value is not a subsection", cur_section.top()->cursor);
        } else {
            if (arrays.debug)
            {
                fprintf(stderr, "Looking for section, got ");
                cur_section.top()->current().print(stderr);
            }
        }
        return plan_sec;
    }

public:
    ArrayBuilder(const Bulletin& bulletin, unsigned subset_no, Arrays& arrays, unsigned bufr_idx)
        : bulletin::UncompressedEncoder(bulletin, subset_no),
          arrays(arrays),
          bufr_idx(bufr_idx),
          prev_code(0), prev_code_count(0)
    {
        cur_section.push(arrays.plan.sections[0]);
        cur_section.top()->cursor = 0;
    }

    void r_replication(Varcode code, Varcode delayed_code, const Opcodes& ops) override
    {
        if (arrays.debug)
            fprintf(stderr, "Begin replicated section %01d%02d%03d/%01d%02d%03d\n",
                    WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code),
                    WR_VAR_F(delayed_code), WR_VAR_X(delayed_code), WR_VAR_Y(delayed_code));

        plan::Section* cur_top = cur_section.top();

        UncompressedEncoder::r_replication(code, delayed_code, ops);

        if (cur_top != cur_section.top())
            // If it iterated, we need to pop the subsection from the stack
            cur_section.pop();

        if (arrays.debug)
        {
            plan::Variable& v = cur_section.top()->current();
            fprintf(stderr, "End replicated section %01d%02d%03d/%01d%02d%03d; next: ",
                    WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code),
                    WR_VAR_F(delayed_code), WR_VAR_X(delayed_code), WR_VAR_Y(delayed_code));
            v.print(stderr);
        }

        // Skip past the subsection
        if (cur_section.top()->current().subsection)
            cur_section.top()->cursor++;
    }

    void run_r_repetition(unsigned cur, unsigned total) override
    {
        if (cur == 0)
        {
            plan::Section* plan_sec = section_for_current_position();
            cur_section.push(plan_sec);
        }
        cur_section.top()->cursor = 0;

        if (arrays.debug)
            fprintf(stderr, "Repetition #%u/%u %u elements\n", cur, total, opcode_stack.top().size());

        UncompressedEncoder::run_r_repetition(cur, total);

        if (arrays.debug)
            fprintf(stderr, "Repetition #%u/%u ended\n", cur, total);
    }

    // Returns the qbit attribute for this variable, or NULL if none is found
    const Var* get_qbits(const Var& var)
    {
        if (const Var* a = var.enqa(WR_VAR(0, 33, 2))) {
            return a;
        } else if (const Var* a = var.enqa(WR_VAR(0, 33, 3))) {
            return a;
        } else if (const Var* a = var.enqa(WR_VAR(0, 33, 50))) {
            return a;
        }
        return NULL;
    }

    /*
    virtual const Var& do_bitmap(Varcode code, Varcode rep_code, Varcode delayed_code, const Opcodes& ops)
    {
        const Var& res = bulletin::ConstBaseVisitor::do_bitmap(code, rep_code, delayed_code, ops);

        // Store the delayed replication factor, if any
        if (delayed_code != 0)
        {
            plan::Variable& v = variable_for(delayed_code);
            if (v.data)
            {
                Var var(btable->query(delayed_code), (int)res.info()->len);
                v.data->add(var, bufr_idx);
            }
        }

        // Store the bitmap itself

        // Fetch the section
        plan::Section* plan_sec = section_for_current_position();
        cur_section.push(plan_sec);
        cur_section.top()->cursor = 0;

        // TODO:   feed it bits

        return res;
    }
    */

    void encode_var(Varinfo info, const Var& var) override
    {
        if (WR_VAR_F(var.code()) == 2 && WR_VAR_X(var.code()) == 6)
            // Skip unknown local descriptors
            return;
        if (info->type == Vartype::Binary)
            // Skip unknown local descriptors
            return;
        plan::Variable& v = variable_for(info->code);
        if (v.data)
        {
            v.data->add(var, bufr_idx);

            // Take note of significant ValArrays
            switch (var.code())
            {
                case WR_VAR(0, 4, 1): if (!arrays.date_year) arrays.date_year = v.data; break;
                case WR_VAR(0, 4, 2): if (!arrays.date_month) arrays.date_month = v.data; break;
                case WR_VAR(0, 4, 3): if (!arrays.date_day) arrays.date_day = v.data; break;
                case WR_VAR(0, 4, 4): if (!arrays.time_hour) arrays.time_hour = v.data; break;
                case WR_VAR(0, 4, 5): if (!arrays.time_minute) arrays.time_minute = v.data; break;
                case WR_VAR(0, 4, 6): if (!arrays.time_second) arrays.time_second = v.data; break;
            }
        }
        if (v.qbits)
            if (const Var* q = get_qbits(var))
                v.qbits->add(*q, bufr_idx);
        cur_section.top()->cursor++;
    }

    unsigned define_bitmap_delayed_replication_factor(Varinfo) override
    {
        const Var& var = peek_var();
        return var.info()->len;
    }

    void define_attribute(Varinfo info, unsigned pos) override
    {
        const Var& var = get_var(pos);
        if (const Var* a = var.enqa(info->code))
            encode_var(info, *a);
        else
        {
            Var attr(info);
            encode_var(info, attr);
        }
    }

    void define_raw_character_data(Varcode) override
    {
        const Var& var = get_var();
        plan::Variable& v = cur_section.top()->current();
        if (v.subsection)
            error_consistency::throwf("out of sync at %u: value is a subsection instead of a variable", cur_section.top()->cursor);
        if (v.data)
        {
            if (WR_VAR_F(v.data->info->code) != WR_VAR_F(var.code())
              || WR_VAR_X(v.data->info->code) != WR_VAR_X(var.code()))
                error_consistency::throwf("out of sync at %u: vars mismatch", cur_section.top()->cursor);
            v.data->add(var, bufr_idx);
        }
#if 0
        ValArray& arr = arrays.get_valarray(Namer::DT_CHAR, var, tag);
        arr.add(var, bufr_idx);
#endif
    }

    void define_c03_refval_override(Varcode) override
    {
        // Nothing to do, it does not make sense in NetCDF, and it should have
        // been handled transparently in the BUFR decoder
    }
};



Arrays::Arrays(const Options& opts)
    : plan(opts),
      date_year(0), date_month(0), date_day(0),
      time_hour(0), time_minute(0), time_second(0),
      date_varid(-1), time_varid(-1),
      verbose(opts.verbose), debug(opts.debug)
{
}

Arrays::~Arrays()
{
    delete first_bulletin;
}

#if 0
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
    unique_ptr<ValArray> arr;
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
#endif

void Arrays::add(unique_ptr<Bulletin>&& bulletin)
{
    if (plan.sections.empty())
    {
        if (debug)
            fprintf(stderr, "\nBuilding conversion plan:\n");
        plan.build(*bulletin);
        if (debug)
        {
            fprintf(stderr, "\nComputed conversion plan:\n");
            plan.print(stderr);
        }
    }

    for (unsigned i = 0; i < bulletin->subsets.size(); ++i)
    {
        ArrayBuilder ab(*bulletin, i, *this, bufr_idx++);
        ab.run();
    }

    if (!first_bulletin)
        first_bulletin = bulletin.release();
}

void Arrays::dump(FILE* out)
{
    plan.print(out);
}

bool Arrays::define(NCOutfile& outfile)
{
    int ncid = outfile.ncid;
    int bufrdim = outfile.dim_bufr_records;

    // Define the array size dimensions
    plan.define(outfile);

    if (date_year && date_month && date_day)
    {
        date_varid = outfile.def_var("DATE", NC_INT, 1, &bufrdim);

        int missing = NC_FILL_INT;
        int res = nc_put_att_int(ncid, date_varid, "_FillValue", NC_INT, 1, &missing);
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
        time_varid = outfile.def_var("TIME", NC_INT, 1, &bufrdim);

        int missing = NC_FILL_INT;
        int res = nc_put_att_int(ncid, time_varid, "_FillValue", NC_INT, 1, &missing);
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
    plan.putvar(outfile);

    int ncid = outfile.ncid;

    if (date_year && date_month && date_day)
    {
        size_t size = date_year->get_size();
        sys::TempBuffer<int> values(size);
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
        sys::TempBuffer<int> values(size);
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

void Sections::add(const wreport::BufrBulletin& bulletin, const std::string& raw)
{
    unsigned start = idx == 0 ? 0 : bulletin.section_end[idx - 1];
    unsigned len = bulletin.section_end[idx] - start;
    if (len == 0)
    {
        values.push_back(string());
        return;
    }

    if (len > max_length)
        max_length = len;

    values.push_back(raw.substr(start, len));
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
    nc_varid = outfile.def_var(name, NC_BYTE, 2, dims);

    return true;
}

void Sections::putvar(NCOutfile& outfile) const
{
    int ncid = outfile.ncid;

    if (max_length == 0)
        return;

    size_t start[] = {0, 0};
    size_t count[] = {1, 0};
    sys::TempBuffer<unsigned char> missing(max_length); // Missing value
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
    int bufrdim = outfile.dim_bufr_records;

    if (values.empty()) return false;
    nc_varid = outfile.def_var(name.c_str(), NC_INT, 1, &bufrdim);

    int missing = NC_FILL_INT;
    int res = nc_put_att_int(outfile.ncid, nc_varid, "_FillValue", NC_INT, 1, &missing);
    error_netcdf::throwf_iferror(res, "creating _FillValue attribute in %s", name.c_str());

    return true;
}

void IntArray::putvar(NCOutfile& outfile) const
{
    if (values.empty()) return;
    size_t start[] = {0};
    size_t count[] = {values.size()};
#ifdef HAVE_VECTOR_DATA
    int res = nc_put_vara_int(outfile.ncid, nc_varid, start, count, values.data());
    error_netcdf::throwf_iferror(res, "storing %zd integer values", values.size());
#else
    int* temp = new int[values.size()];
    for (unsigned i = 0; i < values.size(); ++i)
        temp[i] = values[i];
    int res = nc_put_vara_int(outfile.ncid, nc_varid, start, count, temp);
    error_netcdf::throwf_iferror(res, "storing %zd integer values", values.size());
    delete[] temp;
#endif
}

}
