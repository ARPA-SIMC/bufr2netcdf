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
#include "ncoutfile.h"
#include "plan.h"
#include <wreport/error.h>
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
template<> inline double nc_fill() { return NC_FILL_DOUBLE; }
template<> inline std::string nc_fill() { return string(); }

template<typename TYPE>
static inline nc_type get_nc_type() { throw error_consistency("requested type const value for unknown type"); }
template<> inline nc_type get_nc_type<int>() { return NC_INT; }
template<> inline nc_type get_nc_type<float>() { return NC_FLOAT; }
template<> inline nc_type get_nc_type<double>() { return NC_DOUBLE; }
template<> inline nc_type get_nc_type<std::string>() { return NC_CHAR; }

template<typename TYPE>
static inline void nc_put_att(int ncid, int nc_varid, const char* name, const TYPE& value) { throw error_consistency("nc_put_att called for unknown type"); }
template<> inline void nc_put_att<std::string>(int ncid, int nc_varid, const char* name, const std::string& value)
{
    int res = nc_put_att_text(ncid, nc_varid, name, 1, value.c_str());
    error_netcdf::throwf_iferror(res, "setting %s text attribute", name);
}
template<> inline void nc_put_att<int>(int ncid, int nc_varid, const char* name, const int& value)
{
    int res = nc_put_att_int(ncid, nc_varid, name, NC_INT, 1, &value);
    error_netcdf::throwf_iferror(res, "setting %s int attribute", name);
}
template<> inline void nc_put_att<float>(int ncid, int nc_varid, const char* name, const float& value)
{
    int res = nc_put_att_float(ncid, nc_varid, name, NC_FLOAT, 1, &value);
    error_netcdf::throwf_iferror(res, "setting %s float attribute", name);
}
template<> inline void nc_put_att<double>(int ncid, int nc_varid, const char* name, const double& value)
{
    int res = nc_put_att_double(ncid, nc_varid, name, NC_DOUBLE, 1, &value);
    error_netcdf::throwf_iferror(res, "setting %s double attribute", name);
}


void LoopInfo::define(NCOutfile& outfile, size_t size)
{
    char dn[20];
    snprintf(dn, 20, "Loop_%03d_maxlen", index);
    int res = nc_def_dim(outfile.ncid, dn, size, &nc_dimid);
    error_netcdf::throwf_iferror(res, "creating %s dimension", dn);
}

ValArray::ValArray(wreport::Varinfo info)
    : info(info), master(0), is_constant(true)
{
}

namespace {

struct BaseValArray : public ValArray
{
    BaseValArray(Varinfo info) : ValArray(info) {}

    virtual void add_common_attributes(int ncid)
    {
        int res;

        res = nc_put_att_text(ncid, nc_varid, "long_name", strlen(info->desc), info->desc);
        error_netcdf::throwf_iferror(res, "setting long_name attribute for %s", name.c_str());

        res = nc_put_att_text(ncid, nc_varid, "units", strlen(info->unit), info->unit);
        error_netcdf::throwf_iferror(res, "setting unit attribute for %s", name.c_str());

        res = nc_put_att_text(ncid, nc_varid, "mnemonic", mnemo.size(), mnemo.data());
        error_netcdf::throwf_iferror(res, "setting mnemonic attribute for %s", name.c_str());

        const char* tname = Namer::type_name(type);
        res = nc_put_att_text(ncid, nc_varid, "type", strlen(tname), tname);
        error_netcdf::throwf_iferror(res, "setting type attribute for %s", name.c_str());

        int ifxy;
        if (master)
            ifxy = WR_VAR_F(master->info->code) * 100000 + WR_VAR_X(master->info->code) * 1000 + WR_VAR_Y(master->info->code);
        else
            ifxy = WR_VAR_F(info->code) * 100000 + WR_VAR_X(info->code) * 1000 + WR_VAR_Y(info->code);
        res = nc_put_att_int(ncid, nc_varid, "ifxy", NC_INT, 1, &ifxy);
        error_netcdf::throwf_iferror(res, "setting ifxy attribute for %s", name.c_str());

        {
            int rcnt = this->rcnt;
            res = nc_put_att_int(ncid, nc_varid, "rcnt", NC_INT, 1, &rcnt);
            error_netcdf::throwf_iferror(res, "setting rcnt attribute for %s", name.c_str());
        }

        res = nc_put_att_text(ncid, nc_varid, "dim0_length", strlen("_constant"), "_constant"); // TODO
        error_netcdf::throwf_iferror(res, "setting dim0_length attribute for %s", name.c_str());

        if (!slaves.empty())
        {
            // FIXME: this way of setting string array attributes in NetCDF
            // does not work: if we have to generate NetCDF variables with
            // multiple associated fields, we'll see how to do it

            //const char* fnames[slaves.size()];
            //for (size_t i = 0; i < slaves.size(); ++i)
            //    fnames[i] = slaves[i]->name.c_str();
            //res = nc_put_att_string(ncid, nc_varid, "associated_field", slaves.size(), fnames);
            res = nc_put_att_text(ncid, nc_varid, "associated_field", slaves[0]->name.size(), slaves[0]->name.data());
            error_netcdf::throwf_iferror(res, "setting associated_field attribute for %s", this->name.c_str());
        }

        // Refrences attributes
        if (!references.empty())
        {
            int ref_codes[references.size()];
            unsigned rsize = 0;
            for (size_t i = 0; i < references.size(); ++i)
            {
                Varcode code = references[i].first;
                const ValArray* arr = references[i].second;

                // Do not reference fully undefined arrays
                if (!arr->has_values())
                    continue;

                // Do not reference the same varcode as ourself
                if (arr->info->code == info->code)
                    continue;

                if (WR_VAR_F(code) == 0)
                    ref_codes[rsize++] = WR_VAR_F(code) * 100000
                                       + WR_VAR_X(code) * 1000
                                       + WR_VAR_Y(code);

                char att_name[20];
                if (WR_VAR_F(code) > 0)
                    snprintf(att_name, 20, "reference%d_%01d%02d%03d",
                            WR_VAR_F(code) + 1,
                            0,
                            WR_VAR_X(code),
                            WR_VAR_Y(code));
                else
                    snprintf(att_name, 20, "reference_%01d%02d%03d", WR_VAR_FXY(code));

                res = nc_put_att_text(ncid, nc_varid, att_name, arr->name.size(), arr->name.data());
                error_netcdf::throwf_iferror(res, "setting %s attribute for %s", att_name, this->name.c_str());
            }

            if (rsize > 0)
            {
                res = nc_put_att_int(ncid, nc_varid, "references", NC_INT, rsize, ref_codes);
                error_netcdf::throwf_iferror(res, "setting references attribute for %s", name.c_str());
            }
        }
    }
};

template<typename TYPE>
struct TypedValArray : public BaseValArray
{
    using BaseValArray::BaseValArray;

    void add_common_attributes(int ncid) override
    {
        TYPE missing = nc_fill<TYPE>();
        nc_put_att(ncid, nc_varid, "_FillValue", missing);
        BaseValArray::add_common_attributes(ncid);
    }
};

template<typename TYPE>
struct SingleValArray : public TypedValArray<TYPE>
{
    std::vector<TYPE> vars;
    TYPE last_val;

    SingleValArray(Varinfo info) : TypedValArray<TYPE>(info), last_val(nc_fill<TYPE>()) {}

    size_t get_size() const { return vars.size(); }
    size_t get_max_rep() const { return 1; }

    bool has_values() const
    {
        if (!this->is_constant) return true;
        if (vars.empty()) return false;
        return vars[0] != nc_fill<TYPE>();
    }

    void add(const Var& var, unsigned bufr_idx=0)
    {
        bool is_first = vars.empty();

        while (bufr_idx >= vars.size())
            vars.push_back(nc_fill<TYPE>());
        if (var.isset())
            vars[bufr_idx] = var.enq<TYPE>();

        if (is_first)
            last_val = vars[bufr_idx];
        else if (this->is_constant && last_val != vars[bufr_idx])
            this->is_constant = false;
    }

    Var get_var(unsigned bufr_idx, unsigned rep) const
    {
        Var res(this->info);
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
            fprintf(out, "%s[%zd]: %s\n", this->name.c_str(), i, formatted.c_str());
        }
    }
};

template<typename TYPE>
struct SingleNumberArray : public SingleValArray<TYPE>
{
    SingleNumberArray(Varinfo info) : SingleValArray<TYPE>(info) {}

    bool define(NCOutfile& outfile)
    {
        int bufrdim = outfile.dim_bufr_records;

        // Skip variable if it's never been found
        if (this->vars.empty())
        {
            this->nc_varid = -1;
            return false;
        }

        this->nc_varid = outfile.def_var(this->name.c_str(), get_nc_type<TYPE>(), 1, &bufrdim);

        this->add_common_attributes(outfile.ncid);

        return true;
    }
};

struct SingleIntValArray : public SingleNumberArray<int>
{
    SingleIntValArray(Varinfo info) : SingleNumberArray<int>(info) {}

    void putvar(NCOutfile& outfile) const
    {
        if (vars.empty()) return;
        size_t start[] = {0};
        size_t count[] = {vars.size()};
#ifdef HAVE_VECTOR_DATA
        int res = nc_put_vara_int(outfile.ncid, nc_varid, start, count, vars.data());
        error_netcdf::throwf_iferror(res, "storing %zd integer values", vars.size());
#else
        int* temp = new int[vars.size()];
        for (unsigned i = 0; i < vars.size(); ++i)
            temp[i] = vars[i];
        int res = nc_put_vara_int(outfile.ncid, nc_varid, start, count, temp);
        error_netcdf::throwf_iferror(res, "storing %zd integer values", vars.size());
        delete temp;
#endif
    }
};

struct SingleFloatValArray : public SingleNumberArray<float>
{
    SingleFloatValArray(Varinfo info) : SingleNumberArray<float>(info) {}

    void putvar(NCOutfile& outfile) const
    {
        if (vars.empty()) return;
        size_t start[] = {0};
        size_t count[] = {vars.size()};
#ifdef HAVE_VECTOR_DATA
        int res = nc_put_vara_float(outfile.ncid, nc_varid, start, count, vars.data());
        error_netcdf::throwf_iferror(res, "storing %zd float values", vars.size());
#else
        float* temp = new float[vars.size()];
        for (unsigned i = 0; i < vars.size(); ++i)
            temp[i] = vars[i];
        int res = nc_put_vara_float(outfile.ncid, nc_varid, start, count, temp);
        error_netcdf::throwf_iferror(res, "storing %zd float values", vars.size());
        delete temp;
#endif
    }
};

struct SingleDoubleValArray : public SingleNumberArray<double>
{
    SingleDoubleValArray(Varinfo info) : SingleNumberArray<double>(info) {}

    void putvar(NCOutfile& outfile) const
    {
        if (vars.empty()) return;
        size_t start[] = {0};
        size_t count[] = {vars.size()};
#ifdef HAVE_VECTOR_DATA
        int res = nc_put_vara_double(outfile.ncid, nc_varid, start, count, vars.data());
        error_netcdf::throwf_iferror(res, "storing %zd double values", vars.size());
#else
        double* temp = new double[vars.size()];
        for (unsigned i = 0; i < vars.size(); ++i)
            temp[i] = vars[i];
        int res = nc_put_vara_double(outfile.ncid, nc_varid, start, count, temp);
        error_netcdf::throwf_iferror(res, "storing %zd double values", vars.size());
        delete temp;
#endif
    }
};

struct SingleStringValArray : public SingleValArray<std::string>
{
    SingleStringValArray(Varinfo info) : SingleValArray<std::string>(info) {}

    bool define(NCOutfile& outfile)
    {
        int ncid = outfile.ncid;

        // Skip variable if it's never been found
        if (vars.empty())
        {
            nc_varid = -1;
            return false;
        }

        int dims[2] = { outfile.dim_bufr_records, 0 };
        string dimname = name + "_strlen";
        int res = nc_def_dim(ncid, dimname.c_str(), info->len, &dims[1]);
        error_netcdf::throwf_iferror(res, "creating %s dimension", dimname.c_str());

        nc_varid = outfile.def_var(name.c_str(), NC_CHAR, 2, dims);

        add_common_attributes(ncid);

        return true;
    }

    void putvar(NCOutfile& outfile) const
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
                res = nc_put_vara_text(outfile.ncid, nc_varid, start, count, value);
            } else
                res = nc_put_vara_text(outfile.ncid, nc_varid, start, count, missing);
            error_netcdf::throwf_iferror(res, "storing %zd string values", vars.size());
        }
    }
};


template<typename TYPE>
struct MultiValArray : public TypedValArray<TYPE>
{
    std::vector< std::vector<TYPE> > arrs;
    LoopInfo& loopinfo;
    TYPE last_val;

    MultiValArray(Varinfo info, LoopInfo& loopinfo)
        : TypedValArray<TYPE>(info), loopinfo(loopinfo), last_val(nc_fill<TYPE>()) {}

    bool has_values() const
    {
        if (!this->is_constant) return true;
        // Look for one value
        for (typename std::vector< std::vector<TYPE> >::const_iterator i = arrs.begin();
                i != arrs.end(); ++i)
            for (typename std::vector<TYPE>::const_iterator j = i->begin();
                    j != i->end(); ++j)
                return *j != nc_fill<TYPE>();
        return false;
    }

    void add(const wreport::Var& var, unsigned bufr_idx)
    {
        // Ensure we have the right number of dimensions
        while (bufr_idx >= arrs.size())
            arrs.push_back(vector<TYPE>());

        bool is_first = arrs.empty();

        // Append to the right bufr values
        if (var.isset())
            arrs[bufr_idx].push_back(var.enq<TYPE>());
        else
            arrs[bufr_idx].push_back(nc_fill<TYPE>());

        if (is_first)
            last_val = arrs[bufr_idx].back();
        else if (this->is_constant && last_val != arrs[bufr_idx].back())
            this->is_constant = false;
    }

    Var get_var(unsigned bufr_idx, unsigned rep=0) const
    {
        Var res(this->info);
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

    bool define(NCOutfile& outfile)
    {
        // Skip variable if it's never been found
        if (this->arrs.empty())
        {
            this->nc_varid = -1;
            return false;
        }

        if (loopinfo.nc_dimid == -1)
            loopinfo.define(outfile, get_max_rep());
        return true;
    }


    void dump(FILE* out)
    {
        for (size_t a = 0; a < arrs.size(); ++a)
            for (size_t i = 0; i < arrs[a].size(); ++i)
            {
                Var var = get_var(a, i);
                string formatted = var.format();
                fprintf(out, "%s[%zd,%zd]: %s\n", this->name.c_str(), a, i, formatted.c_str());
            }
    }
};

template<typename TYPE>
struct MultiNumberValArray : public MultiValArray<TYPE>
{
    MultiNumberValArray(Varinfo info, LoopInfo& loopinfo)
        : MultiValArray<TYPE>(info, loopinfo) {}

    bool define(NCOutfile& outfile)
    {
        if (!MultiValArray<TYPE>::define(outfile))
            return false;

        int ncid = outfile.ncid;

        int dims[] = { outfile.dim_bufr_records, this->loopinfo.nc_dimid };
        this->nc_varid = outfile.def_var(this->name.c_str(), get_nc_type<TYPE>(), 2, dims);

        this->add_common_attributes(ncid);

        // Only set to non-const if it changes across BUFRs
        string dimlenname = "_constant";
        if (this->loopinfo.var && !this->loopinfo.var->is_constant)
            dimlenname = this->loopinfo.var->name;
        int res = nc_put_att_text(ncid, this->nc_varid, "dim1_length", dimlenname.size(), dimlenname.data());
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
#ifdef HAVE_VECTOR_DATA
        if (vals.size() < storage_size)
        {
            memcpy(storage, vals.data(), vals.size() * sizeof(TYPE));
            for (size_t i = vals.size(); i < storage_size; ++i)
                storage[i] = nc_fill<TYPE>();
            return storage;
        } else
            return vals.data();
#else
        for (unsigned i = 0; i < vals.size(); ++i)
            ((TYPE*)storage)[i] = vals[i];
        for (size_t i = vals.size(); i < storage_size; ++i)
            storage[i] = nc_fill<TYPE>();
        return storage;
#endif
    }
};

struct MultiIntValArray : public MultiNumberValArray<int>
{
    MultiIntValArray(Varinfo info, LoopInfo& loopinfo)
        : MultiNumberValArray<int>(info, loopinfo) {}

    void putvar(NCOutfile& outfile) const
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
            int res = nc_put_vara_int(outfile.ncid, nc_varid, start, count, to_nc);
            error_netcdf::throwf_iferror(res, "storing %zd int values", arrsize);
        }
    }
};

struct MultiFloatValArray : public MultiNumberValArray<float>
{
    MultiFloatValArray(Varinfo info, LoopInfo& loopinfo)
        : MultiNumberValArray<float>(info, loopinfo) {}

    void putvar(NCOutfile& outfile) const
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
            int res = nc_put_vara_float(outfile.ncid, nc_varid, start, count, to_nc);
            error_netcdf::throwf_iferror(res, "storing %zd values", arrsize);
        }
    }
};

struct MultiDoubleValArray : public MultiNumberValArray<double>
{
    MultiDoubleValArray(Varinfo info, LoopInfo& loopinfo)
        : MultiNumberValArray<double>(info, loopinfo) {}

    void putvar(NCOutfile& outfile) const
    {
        if (arrs.empty()) return;

        size_t arrsize = get_max_rep();
        size_t start[] = {0, 0};
        size_t count[] = {1, arrsize};
        double clean_vals[arrsize];

        for (unsigned i = 0; i < arrs.size(); ++i)
        {
            const double* to_nc = to_fixed_array(i, clean_vals, arrsize);
            start[0] = i;
            int res = nc_put_vara_double(outfile.ncid, nc_varid, start, count, to_nc);
            error_netcdf::throwf_iferror(res, "storing %zd values", arrsize);
        }
    }
};

struct MultiStringValArray : public MultiValArray<std::string>
{
    MultiStringValArray(Varinfo info, LoopInfo& loopinfo)
        : MultiValArray<std::string>(info, loopinfo) {}

    bool define(NCOutfile& outfile)
    {
        if (!MultiValArray<std::string>::define(outfile))
            return false;

        int ncid = outfile.ncid;

        int dims[3] = { outfile.dim_bufr_records, loopinfo.nc_dimid, 0 };
        string dimname = name + "_strlen";
        int res = nc_def_dim(ncid, dimname.c_str(), info->len, &dims[2]);
        error_netcdf::throwf_iferror(res, "creating %s dimension", dimname.c_str());

        nc_varid = outfile.def_var(name.c_str(), NC_CHAR, 3, dims);

        add_common_attributes(ncid);

        // Only set to non-const if it changes across BUFRs
        string dimlenname = "_constant";
        if (this->loopinfo.var && !this->loopinfo.var->is_constant)
            dimlenname = this->loopinfo.var->name;
        res = nc_put_att_text(ncid, this->nc_varid, "dim1_length", dimlenname.size(), dimlenname.data());
        error_netcdf::throwf_iferror(res, "setting dim1_length attribute for %s", this->name.c_str());

        return true;
    }

    void putvar(NCOutfile& outfile) const
    {
        if (arrs.empty()) return;

        size_t arrsize = get_max_rep();
        size_t start[] = {0, 0, 0};
        size_t count[] = {1, 1, info->len};

        char missing[info->len]; // Missing value
        memset(missing, NC_FILL_CHAR, info->len);

        char value[info->len]; // Space-padded value
        for (size_t i = 0; i < arrs.size(); ++i)
        {
            const vector<string>& v = arrs[i];
            start[0] = i;
            for (size_t j = 0; j < arrsize; ++j)
            {
                start[1] = j;
                int res;
                if (j >= v.size() || v[j].empty())
                    res = nc_put_vara_text(outfile.ncid, nc_varid, start, count, missing);
                else
                {
                    memcpy(value, v[j].data(), v[j].size());
                    for (size_t k = v[j].size(); k < info->len; ++k)
                        value[k] = ' ';
                    res = nc_put_vara_text(outfile.ncid, nc_varid, start, count, value);
                }
                error_netcdf::throwf_iferror(res, "storing %zd string values", arrsize);
            }
        }
    }
};

}

ValArray* ValArray::make_singlevalarray(Namer::DataType type, Varinfo info)
{
    switch (type)
    {
        case Namer::DT_DATA:
            switch (info->type)
            {
                case Vartype::String:
                    return new SingleStringValArray(info);
                case Vartype::Integer:
                    return new SingleIntValArray(info);
                case Vartype::Decimal:
                    // Hardcoding as prototype for addressing https://github.com/ARPA-SIMC/bufr2netcdf/issues/10
                    if (info->code == WR_VAR(0, 5, 1) or info->code == WR_VAR(0, 6, 1))
                        return new SingleDoubleValArray(info);
                    else
                        return new SingleFloatValArray(info);
                default:
                    error_unimplemented::throwf("cannot export variables of type '%s'", vartype_format(info->type));
            }
        case Namer::DT_QBITS:
            return new SingleIntValArray(info);
        case Namer::DT_CHAR:
            return new SingleStringValArray(info);
        default:
            throw error_unimplemented("only Data, QBits and Char storages are supported so far");
    }
}

ValArray* ValArray::make_multivalarray(Namer::DataType type, Varinfo info, LoopInfo& loopinfo)
{
    switch (type)
    {
        case Namer::DT_DATA:
            switch (info->type)
            {
                case Vartype::String:
                    return new MultiStringValArray(info, loopinfo);
                case Vartype::Integer:
                    return new MultiIntValArray(info, loopinfo);
                case Vartype::Decimal:
                    // Hardcoding as prototype for addressing https://github.com/ARPA-SIMC/bufr2netcdf/issues/10
                    if (info->code == WR_VAR(0, 5, 1) or info->code == WR_VAR(0, 6, 1))
                        return new MultiDoubleValArray(info, loopinfo);
                    else
                        return new MultiFloatValArray(info, loopinfo);
                default:
                    error_unimplemented::throwf("cannot export variables of type '%s'", vartype_format(info->type));
            }
        case Namer::DT_QBITS:
            return new MultiIntValArray(info, loopinfo);
        case Namer::DT_CHAR:
            return new MultiStringValArray(info, loopinfo);
        default:
            throw error_unimplemented("only Data, QBits and Char storages are supported so far");
    }
}

}
