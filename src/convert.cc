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
#include "arrays.h"
#include "utils.h"
#include <wreport/bulletin.h>
#include <map>
#include <vector>
#include <netcdf.h>

using namespace wreport;
using namespace std;

namespace b2nc {

void read_bufr(const std::string& fname, BufrSink& out)
{
    // Open input file
    FILE* in = fopen(fname.c_str(), "rb");
    if (in == NULL)
        error_system::throwf("cannot open %s", fname.c_str());

    try {
        read_bufr(in, out, fname.c_str());
        fclose(in);
    } catch (...) {
        fclose(in);
        throw;
    }
}

void read_bufr(FILE* in, BufrSink& out, const char* fname)
{
    string rawmsg;
    BufrBulletin bulletin;

    while (BufrBulletin::read(in, rawmsg, fname))
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);

        out.add_bufr(bulletin);
    }
}


Outfile::Outfile() : ncid(-1) {}
Outfile::~Outfile()
{
    if (ncid != -1)
        close();
}

void Outfile::open(const std::string& fname)
{
    // Create output file
    this->fname = fname;
    int res = nc_create(fname.c_str(), NC_CLOBBER, &ncid);
    error_netcdf::throwf_iferror(res, "creating file %s", fname.c_str());
}

void Outfile::close()
{
    if (ncid != -1)
    {
        nc_close(ncid);
        ncid = -1;
    }
}

struct OutfileImpl : public Outfile
{
    Arrays arrays;
    Sections sec1;
    Sections sec2;
    IntArray edition;
    IntArray s1mtn;
    IntArray s1ce;
    IntArray s1sc;
    IntArray s1usn;
    IntArray s1cat;
    IntArray s1subcat;
    IntArray s1localsubcat;
    IntArray s1mtv;
    IntArray s1ltv;
    IntArray s1date;
    IntArray s1time;

    OutfileImpl(const Options& opts)
        : arrays(opts.use_mnemonic ? Namer::MNEMONIC : Namer::PLAIN),
          sec1(1), sec2(2),
          edition("edition_number"),
          s1mtn("section1_master_table_nr"),
          s1ce("section1_centre"),
          s1sc("section1_subcentre"),
          s1usn("section1_update_sequence_nr"),
          s1cat("section1_data_category"),
          s1subcat("section1_int_data_sub_category"),
          s1localsubcat("section1_local_data_sub_category"),
          s1mtv("section1_master_tables_version"),
          s1ltv("section1_local_tables_version"),
          s1date("section1_date"),
          s1time("section1_time")
    {
    }

    virtual void add_bufr(const wreport::BufrBulletin& bulletin)
    {
        // TODO: if first, build metadata

        arrays.add(bulletin);

        for (vector<Subset>::const_iterator si = bulletin.subsets.begin();
                si != bulletin.subsets.end(); ++si)
        {
            // Add contents to the various data arrays
            edition.add(bulletin.edition);
            s1mtn.add(bulletin.master_table_number);
            s1ce.add(bulletin.centre);
            s1sc.add(bulletin.subcentre);
            s1usn.add(bulletin.update_sequence_number);
            s1cat.add(bulletin.type);
            if (bulletin.subtype == 255)
                s1subcat.add_missing();
            else
                s1subcat.add(bulletin.subtype);
            s1localsubcat.add(bulletin.localsubtype);
            s1mtv.add(bulletin.master_table);
            s1ltv.add(bulletin.local_table);
            s1date.add(bulletin.rep_year * 10000 + bulletin.rep_month * 100 + bulletin.rep_day);
            s1time.add(bulletin.rep_hour * 10000 + bulletin.rep_minute * 100 + bulletin.rep_second);
            sec1.add(bulletin);
            sec2.add(bulletin);
        }
    }

    virtual void close()
    {
        // TODO: add arrays to NetCDF
        int res;

        // Define dimensions

        int dim_bufr_records;
        res = nc_def_dim(ncid, "BUFR_records", NC_UNLIMITED, &dim_bufr_records);
        error_netcdf::throwf_iferror(res, "creating BUFR_records dimension for file %s", "##TODO##");

        // Define variables

        edition.define(ncid, dim_bufr_records);
        s1mtn.define(ncid, dim_bufr_records);
        s1ce.define(ncid, dim_bufr_records);
        s1sc.define(ncid, dim_bufr_records);
        s1usn.define(ncid, dim_bufr_records);
        s1cat.define(ncid, dim_bufr_records);
        s1subcat.define(ncid, dim_bufr_records);
        s1localsubcat.define(ncid, dim_bufr_records);
        s1mtv.define(ncid, dim_bufr_records);
        s1ltv.define(ncid, dim_bufr_records);
        s1date.define(ncid, dim_bufr_records);
        s1time.define(ncid, dim_bufr_records);
        sec1.define(ncid, dim_bufr_records);
        sec2.define(ncid, dim_bufr_records);
        arrays.define(ncid, dim_bufr_records);

        // TODO nc_put_att       /* put attribute: assign attribute values */

        // End define mode
        res = nc_enddef(ncid);
        error_netcdf::throwf_iferror(res, "leaving define mode for file %s", "##TODO##");

        edition.putvar(ncid);
        s1mtn.putvar(ncid);
        s1ce.putvar(ncid);
        s1sc.putvar(ncid);
        s1usn.putvar(ncid);
        s1cat.putvar(ncid);
        s1subcat.putvar(ncid);
        s1localsubcat.putvar(ncid);
        s1mtv.putvar(ncid);
        s1ltv.putvar(ncid);
        s1date.putvar(ncid);
        s1time.putvar(ncid);
        sec1.putvar(ncid);
        sec2.putvar(ncid);
        arrays.putvar(ncid);

        Outfile::close();
    }
};

std::auto_ptr<Outfile> Outfile::get(const Options& opts)
{
    return auto_ptr<Outfile>(new OutfileImpl(opts));
}


Dispatcher::Dispatcher(const Options& opts)
    : opts(opts)
{
}

Dispatcher::~Dispatcher()
{
    close();
}

void Dispatcher::close()
{
    for (std::map< std::vector<wreport::Varcode>, Outfile* >::iterator i = outfiles.begin();
            i != outfiles.end(); ++i)
    {
        i->second->close();
        delete i->second;
    }
    outfiles.clear();
}

std::string Dispatcher::get_fname(const wreport::BufrBulletin& bulletin)
{
    string base = opts.out_fname;

    // Drop the trailing .nc extension if it exists
    if (base.substr(base.size() - 3) == ".nc")
        base = base.substr(0, base.size() - 3);

    char catstr[40];
    snprintf(catstr, 40, "-%d-%d-%d", bulletin.type, bulletin.subtype, bulletin.localsubtype);
    base += catstr;

    string cand = base + ".nc";
    for (unsigned i = 0; used_fnames.find(cand) != used_fnames.end(); ++i)
    {
        char ext[20];
        snprintf(ext, 20, "-%d.nc", i);
        cand = base + ext;
    }
    used_fnames.insert(cand);
    return cand;
}

Outfile& Dispatcher::get_outfile(const wreport::BufrBulletin& bulletin)
{
    std::map< std::vector<wreport::Varcode>, Outfile* >::iterator i = outfiles.find(bulletin.datadesc);
    if (i != outfiles.end())
        return *(i->second);
    else
    {
        auto_ptr<Outfile> out = Outfile::get(opts);
        Outfile& res = *out;
        out->open(get_fname(bulletin));
        outfiles.insert(make_pair(bulletin.datadesc, out.release()));
        return res;
    }
}

void Dispatcher::add_bufr(const wreport::BufrBulletin& bulletin)
{
    get_outfile(bulletin).add_bufr(bulletin);
}

}
