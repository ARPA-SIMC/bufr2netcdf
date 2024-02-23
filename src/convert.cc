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
#include "options.h"
#include "ncoutfile.h"
#include "arrays.h"
#include "utils.h"
#include <wreport/bulletin.h>
#include <map>
#include <vector>
#include <netcdf.h>

using namespace wreport;
using namespace std;

namespace b2nc {

struct OutfileImpl;

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
    unique_ptr<BufrCodecOptions> codec_opts(BufrCodecOptions::create());
    codec_opts->decode_adds_undef_attrs = true;

    off_t offset;
    while (BufrBulletin::read(in, rawmsg, fname, &offset))
    {
        // Decode the BUFR message
        unique_ptr<BufrBulletin> bulletin = BufrBulletin::decode(rawmsg, *codec_opts, fname, offset);

        out.add_bufr(move(bulletin), rawmsg);
    }
}

Dispatcher::Key::Key(const wreport::BufrBulletin& bulletin)
    : type(bulletin.data_category),
      subtype(bulletin.data_subcategory),
      localsubtype(bulletin.data_subcategory_local),
      master_table_version_number(bulletin.master_table_version_number),
      datadesc(bulletin.datadesc)
{
}

bool Dispatcher::Key::operator<(const Key& v) const
{
    if (type < v.type) return true;
    if (type > v.type) return false;
    if (subtype < v.subtype) return true;
    if (subtype > v.subtype) return false;
    if (localsubtype < v.localsubtype) return true;
    if (localsubtype > v.localsubtype) return false;
    if (master_table_version_number < v.master_table_version_number) return true;
    if (master_table_version_number > v.master_table_version_number) return false;
    return datadesc < v.datadesc;
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
    for (std::map<Key, Outfile*>::iterator i = outfiles.begin();
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
    if (base.length() > 2)
        if (base.substr(base.size() - 3) == ".nc")
            base = base.substr(0, base.size() - 3);

    char catstr[40];
    snprintf(catstr, 40, "-%d-%d-%d",
            bulletin.data_category, bulletin.data_subcategory,
            bulletin.data_subcategory_local);
    base += catstr;

    string cand = base + ".nc";
    for (unsigned i = 1; used_fnames.find(cand) != used_fnames.end(); ++i)
    {
        char ext[20];
        snprintf(ext, 20, ".%d.nc", i);
        cand = base + ext;
    }
    used_fnames.insert(cand);
    return cand;
}

Outfile& Dispatcher::get_outfile(const wreport::BufrBulletin& bulletin)
{
    Key key(bulletin);
    std::map<Key, Outfile*>::iterator i = outfiles.find(key);
    if (i != outfiles.end())
        return *(i->second);
    else
    {
        unique_ptr<Outfile> out = Outfile::get(opts);
        Outfile& res = *out;
        out->open(get_fname(bulletin));
        outfiles.insert(make_pair(key, out.release()));
        return res;
    }
}

void Dispatcher::add_bufr(unique_ptr<wreport::BufrBulletin>&& bulletin, const std::string& raw)
{
    get_outfile(*bulletin).add_bufr(move(bulletin), raw);
}

/**
 * Collect data from BUFR bulletins, organise them in NetCDF-style arrays, and
 * write a NetCDF file with the resulting arrays
 */
struct NCFiller
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

    NCFiller(const Options& opts)
        : arrays(opts),
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
        //arrays.debug = true;
    }

    void add(unique_ptr<BufrBulletin>&& bulletin, const std::string& raw)
    {
        for (auto& si: bulletin->subsets)
        {
            // Add contents to the various data arrays
            edition.add(bulletin->edition_number);
            s1mtn.add(bulletin->master_table_number);
            s1ce.add(bulletin->originating_centre);
            s1sc.add(bulletin->originating_subcentre);
            s1usn.add(bulletin->update_sequence_number);
            s1cat.add(bulletin->data_category);
            if (bulletin->data_subcategory == 255)
                s1subcat.add_missing();
            else
                s1subcat.add(bulletin->data_subcategory);
            s1localsubcat.add(bulletin->data_subcategory_local);
            s1mtv.add(bulletin->master_table_version_number);
            s1ltv.add(bulletin->master_table_version_number_local);
            s1date.add(bulletin->rep_year * 10000 + bulletin->rep_month * 100 + bulletin->rep_day);
            s1time.add(bulletin->rep_hour * 10000 + bulletin->rep_minute * 100 + bulletin->rep_second);
            sec1.add(*bulletin, raw);
            sec2.add(*bulletin, raw);
        }

        arrays.add(move(bulletin));
    }

    void define(NCOutfile& outfile)
    {
        // Define variables

        edition.define(outfile);
        s1mtn.define(outfile);
        s1ce.define(outfile);
        s1sc.define(outfile);
        s1usn.define(outfile);
        s1cat.define(outfile);
        s1subcat.define(outfile);
        s1localsubcat.define(outfile);
        s1mtv.define(outfile);
        s1ltv.define(outfile);
        s1date.define(outfile);
        s1time.define(outfile);
        sec1.define(outfile);
        sec2.define(outfile);
        arrays.define(outfile);
    }

    void putvar(NCOutfile& outfile)
    {
        edition.putvar(outfile);
        s1mtn.putvar(outfile);
        s1ce.putvar(outfile);
        s1sc.putvar(outfile);
        s1usn.putvar(outfile);
        s1cat.putvar(outfile);
        s1subcat.putvar(outfile);
        s1localsubcat.putvar(outfile);
        s1mtv.putvar(outfile);
        s1ltv.putvar(outfile);
        s1date.putvar(outfile);
        s1time.putvar(outfile);
        sec1.putvar(outfile);
        sec2.putvar(outfile);
        arrays.putvar(outfile);
    }
};

struct OutfileImpl : public Outfile
{
    NCFiller filler;
    NCOutfile ncout;

    OutfileImpl(const Options& opts)
        : filler(opts), ncout(opts)
    {
    }

    ~OutfileImpl()
    {
        close();
    }

    void open(const std::string& fname)
    {
        ncout.open(fname);
    }

    void close()
    {
        if (ncout.ncid == -1)
            return;

        try {
            // Define all other dimensions, variables and attributes
            filler.define(ncout);

            // End define mode
            ncout.end_define_mode();

            // Put variables
            filler.putvar(ncout);

            ncout.close();
        } catch (...) {
            // Close the file anyway in case of error, so we don't try to write
            // things out again in the destructor
            ncout.close();
            throw;
        }
    }

    void add_bufr(std::unique_ptr<wreport::BufrBulletin>&& bulletin, const std::string& raw) override
    {
        filler.add(move(bulletin), raw);
    }
};

std::unique_ptr<Outfile> Outfile::get(const Options& opts)
{
    return unique_ptr<Outfile>(new OutfileImpl(opts));
}

}
