/*
 * valarray - Storage for plain variable values
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

#include "valarray.h"
#include "utils.h"
#include "mnemo.h"
#include <wreport/var.h>
#include <netcdf.h>
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {

template<typename TYPE>
static inline TYPE nc_fill() { throw error_consistency("requested fill value for unknown type"); }
template<> inline int nc_fill() { return NC_FILL_INT; }
template<> inline float nc_fill() { return NC_FILL_FLOAT; }
template<> inline std::string nc_fill() { return string(); }

template<typename TYPE>
static inline int nc_type() { throw error_consistency("requested type const value for unknown type"); }
template<> inline int nc_type<int>() { return NC_INT; }
template<> inline int nc_type<float>() { return NC_FLOAT; }
template<> inline int nc_type<std::string>() { return NC_CHAR; }


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

template<typename TYPE>
struct SingleValArray : public BaseValArray
{
    std::vector<TYPE> vars;

    SingleValArray(Varinfo info) : BaseValArray(info) {}

    size_t get_size() const { return vars.size(); }
    size_t get_max_rep() const { return 1; }

    void add(const Var& var, unsigned bufr_idx=0)
    {
        while (bufr_idx >= vars.size())
            vars.push_back(nc_fill<TYPE>());
        if (var.isset())
            vars[bufr_idx] = var.enq<TYPE>();
    }

    Var get_var(unsigned bufr_idx, unsigned rep) const
    {
        Var res(info);
        if (rep == 0 && bufr_idx < vars.size() && vars[bufr_idx] != nc_fill<TYPE>())
            res.set(vars[bufr_idx]);
        return res;
    }

    void dump(FILE* out)
    {
        for (size_t i = 0; i < vars.size(); ++i)
        {
            Var var = get_var(i, 0);
            string formatted = var.format();
            fprintf(out, "%s[%zd]: %s\n", name.c_str(), i, formatted.c_str());
        }
    }
};

template<typename TYPE>
struct SingleNumberArray : public SingleValArray<TYPE>
{
    SingleNumberArray(Varinfo info) : SingleValArray<TYPE>(info) {}

    bool define(int ncid, int bufrdim)
    {
        // Skip variable if it's never been found
        if (this->vars.empty())
        {
            this->nc_varid = -1;
            return false;
        }

        int res = nc_def_var(ncid, this->name.c_str(), nc_type<TYPE>(), 1, &bufrdim, &this->nc_varid);
        error_netcdf::throwf_iferror(res, "creating variable %s", this->name.c_str());

        this->add_common_attributes(ncid);

        return this->nc_varid;
    }
};

struct SingleIntValArray : public SingleNumberArray<int>
{
    SingleIntValArray(Varinfo info) : SingleNumberArray<int>(info) {}

    void putvar(int ncid) const
    {
        if (vars.empty()) return;
        size_t start[] = {0};
        size_t count[] = {vars.size()};
        int res = nc_put_vara_int(ncid, nc_varid, start, count, vars.data());
        error_netcdf::throwf_iferror(res, "storing %zd integer values", vars.size());
    }
};

struct SingleFloatValArray : public SingleNumberArray<float>
{
    SingleFloatValArray(Varinfo info) : SingleNumberArray<float>(info) {}

    void putvar(int ncid) const
    {
        if (vars.empty()) return;
        size_t start[] = {0};
        size_t count[] = {vars.size()};
        int res = nc_put_vara_float(ncid, nc_varid, start, count, vars.data());
        error_netcdf::throwf_iferror(res, "storing %zd float values", vars.size());
    }
};

struct SingleStringValArray : public SingleValArray<std::string>
{
    SingleStringValArray(Varinfo info) : SingleValArray<std::string>(info) {}

    bool define(int ncid, int bufrdim)
    {
        // Skip variable if it's never been found
        if (vars.empty())
        {
            nc_varid = -1;
            return false;
        }

        int dims[2] = { bufrdim, 0 };
        string dimname = name + "_strlen";
        int res = nc_def_dim(ncid, dimname.c_str(), info->len, &dims[1]);
        error_netcdf::throwf_iferror(res, "creating %s dimension", dimname.c_str());

        res = nc_def_var(ncid, name.c_str(), NC_CHAR, 2, dims, &nc_varid);
        error_netcdf::throwf_iferror(res, "creating variable %s", name.c_str());

        add_common_attributes(ncid);

        return nc_varid;
    }

    void putvar(int ncid) const
    {
        if (vars.empty()) return;

        size_t start[] = {0, 0};
        size_t count[] = {1, info->len};
        char missing[info->len]; // Missing value
        memset(missing, NC_FILL_CHAR, info->len);
        char value[info->len]; // Space-padded value
        for (size_t i = 0; i < vars.size(); ++i)
        {
            int res;
            start[0] = i;
            if (!vars[i].empty())
            {
                size_t len = vars[i].size();
                memcpy(value, vars[i].data(), len);
                for (size_t i = len; i < info->len; ++i)
                    value[i] = ' ';
                res = nc_put_vara_text(ncid, nc_varid, start, count, value);
            } else
                res = nc_put_vara_text(ncid, nc_varid, start, count, missing);
            error_netcdf::throwf_iferror(res, "storing %zd string values", vars.size());
        }
    }
};


template<typename TYPE>
struct MultiValArray : public BaseValArray
{
    std::vector< std::vector<TYPE> > arrs;
    const LoopInfo& loopinfo;

    MultiValArray(Varinfo info, const LoopInfo& loopinfo)
        : BaseValArray(info), loopinfo(loopinfo) {}

    void add(const wreport::Var& var, unsigned bufr_idx)
    {
        // Ensure we have the right number of dimensions
        while (bufr_idx >= arrs.size())
            arrs.push_back(vector<TYPE>());

        // Append to the right bufr values
        if (var.isset())
            arrs[bufr_idx].push_back(var.enq<TYPE>());
        else
            arrs[bufr_idx].push_back(nc_fill<TYPE>());
    }

    Var get_var(unsigned bufr_idx, unsigned rep=0) const
    {
        Var res(info);
        if (bufr_idx < arrs.size() && rep < arrs[bufr_idx].size() && arrs[bufr_idx][rep] != nc_fill<TYPE>())
            res.set(arrs[bufr_idx][rep]);
        return res;
    }

    size_t get_size() const
    {
        return arrs.size();
    }

    size_t get_max_rep() const
    {
        size_t res = 0;
        for (typename std::vector< std::vector<TYPE> >::const_iterator i = this->arrs.begin();
                i != this->arrs.end(); ++i)
        {
            size_t s = i->size();
            if (s > res) res = s;
        }
        return res;
    }

    void dump(FILE* out)
    {
        for (size_t a = 0; a < arrs.size(); ++a)
            for (size_t i = 0; i < arrs[a].size(); ++i)
            {
                Var var = get_var(a, i);
                string formatted = var.format();
                fprintf(out, "%s[%zd,%zd]: %s\n", name.c_str(), a, i, formatted.c_str());
            }
    }
};

template<typename TYPE>
struct MultiNumberValArray : public MultiValArray<TYPE>
{
    MultiNumberValArray(Varinfo info, const LoopInfo& loopinfo)
        : MultiValArray<TYPE>(info, loopinfo) {}

    bool define(int ncid, int bufrdim)
    {
        // Skip variable if it's never been found
        if (this->arrs.empty())
        {
            this->nc_varid = -1;
            return false;
        }

        int dims[3] = { bufrdim, this->loopinfo.nc_dimid };
        int res = nc_def_var(ncid, this->name.c_str(), nc_type<TYPE>(), 2, dims, &this->nc_varid);
        error_netcdf::throwf_iferror(res, "creating variable %s", this->name.c_str());

        this->add_common_attributes(ncid);

        res = nc_put_att_text(ncid, this->nc_varid, "dim1_length", strlen("_constant"), "_constant"); // TODO
        error_netcdf::throwf_iferror(res, "setting dim1_length attribute for %s", this->name.c_str());

        return true;
    }

    /**
     * If arrs[arr_idx].size() is as long as \a storage_size, return a pointer
     * to its data.
     *
     * Else, copy its values to \a storage, padding with fill values, and
     * returns \a storage
     */
    const TYPE* to_fixed_array(size_t arr_idx, TYPE* storage, size_t storage_size) const
    {
        const vector<TYPE>& vals = this->arrs[arr_idx];
        if (vals.size() < storage_size)
        {
            memcpy(storage, vals.data(), vals.size() * sizeof(TYPE));
            for (size_t i = vals.size(); i < storage_size; ++i)
                storage[i] = nc_fill<TYPE>();
            return storage;
        } else
            return vals.data();
    }
};

struct MultiIntValArray : public MultiNumberValArray<int>
{
    MultiIntValArray(Varinfo info, const LoopInfo& loopinfo)
        : MultiNumberValArray<int>(info, loopinfo) {}

    void putvar(int ncid) const
    {
        if (arrs.empty()) return;

        size_t arrsize = get_max_rep();
        size_t start[] = {0, 0};
        size_t count[] = {1, arrsize};
        int clean_vals[arrsize];

        for (unsigned i = 0; i < arrs.size(); ++i)
        {
            const int* to_nc = to_fixed_array(i, clean_vals, arrsize);
            start[0] = i;
            int res = nc_put_vara_int(ncid, nc_varid, start, count, to_nc);
            error_netcdf::throwf_iferror(res, "storing %zd int values", arrsize);
        }
    }
};

struct MultiFloatValArray : public MultiNumberValArray<float>
{
    MultiFloatValArray(Varinfo info, const LoopInfo& loopinfo)
        : MultiNumberValArray<float>(info, loopinfo) {}

    void putvar(int ncid) const
    {
        if (arrs.empty()) return;

        size_t arrsize = get_max_rep();
        size_t start[] = {0, 0};
        size_t count[] = {1, arrsize};
        float clean_vals[arrsize];

        for (unsigned i = 0; i < arrs.size(); ++i)
        {
            const float* to_nc = to_fixed_array(i, clean_vals, arrsize);
            start[0] = i;
            int res = nc_put_vara_float(ncid, nc_varid, start, count, to_nc);
            error_netcdf::throwf_iferror(res, "storing %zd values", arrsize);
        }
    }
};

struct MultiStringValArray : public MultiValArray<std::string>
{
    MultiStringValArray(Varinfo info, const LoopInfo& loopinfo)
        : MultiValArray<std::string>(info, loopinfo) {}

    bool define(int ncid, int bufrdim)
    {
        // Skip variable if it's never been found
        if (arrs.empty())
        {
            nc_varid = -1;
            return false;
        }

        int dims[3] = { bufrdim, loopinfo.nc_dimid, 0 };
        string dimname = name + "_strlen";
        int res = nc_def_dim(ncid, dimname.c_str(), info->len, &dims[2]);
        error_netcdf::throwf_iferror(res, "creating %s dimension", dimname.c_str());

        res = nc_def_var(ncid, name.c_str(), NC_CHAR, 3, dims, &nc_varid);
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
        size_t count[] = {1, arrsize, info->len};

        char missing[info->len]; // Missing value
        memset(missing, NC_FILL_CHAR, info->len);

        char value[info->len]; // Space-padded value
        for (size_t i = 0; i < arrs.size(); ++i)
        {
            const vector<string>& v = arrs[i];
            start[0] = i;
            for (size_t j = 0; j < arrsize; ++j)
            {
                int res;
                if (j >= v.size() || v[j].empty())
                    res = nc_put_vara_text(ncid, nc_varid, start, count, missing);
                else
                {
                    memcpy(value, v[j].data(), v[j].size());
                    for (size_t k = v[j].size(); k < info->len; ++k)
                        value[k] = ' ';
                    res = nc_put_vara_text(ncid, nc_varid, start, count, value);
                }
                error_netcdf::throwf_iferror(res, "storing %zd string values", arrsize);
            }
        }
    }
};


ValArray* ValArray::make_singlevalarray(Varinfo info)
{
    if (info->is_string())
        return new SingleStringValArray(info);
    else if (info->scale == 0)
        return new SingleIntValArray(info);
    else
        return new SingleFloatValArray(info);
}

ValArray* ValArray::make_multivalarray(Varinfo info, const LoopInfo& loopinfo)
{
    if (info->is_string())
        return new MultiStringValArray(info, loopinfo);
    else if (info->scale == 0)
        return new MultiIntValArray(info, loopinfo);
    else
        return new MultiFloatValArray(info, loopinfo);
}

}
