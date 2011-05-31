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

void Converter::convert(FILE* in, int outncid)
{
    string rawmsg;
    BufrBulletin bulletin;

    Arrays arrays(Namer::MNEMONIC);
    Sections sec1(1);
    Sections sec2(2);
    IntArray edition("edition_number");
    IntArray s1mtn("section1_master_table_nr");
    IntArray s1ce("section1_centre");
    IntArray s1sc("section1_subcentre");
    IntArray s1usn("section1_update_sequence_nr");
    IntArray s1cat("section1_data_category");
    IntArray s1subcat("section1_int_data_sub_category");
    IntArray s1localsubcat("section1_local_data_sub_category");
    IntArray s1mtv("section1_master_tables_version");
    IntArray s1ltv("section1_local_tables_version");
    IntArray s1date("section1_date");
    IntArray s1time("section1_time");

    int bufr_idx = -1;
    while (BufrBulletin::read(in, rawmsg /* , fname = 0 */))
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);

        // TODO: if first, build metadata

        arrays.add(bulletin, bufr_idx);

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

    // TODO: add arrays to NetCDF
    int res;

    // Define dimensions

    int dim_bufr_records;
    res = nc_def_dim(outncid, "BUFR_records", NC_UNLIMITED, &dim_bufr_records);
    error_netcdf::throwf_iferror(res, "creating BUFR_records dimension for file %s", "##TODO##");

    // Define variables

    edition.define(outncid, dim_bufr_records);
    s1mtn.define(outncid, dim_bufr_records);
    s1ce.define(outncid, dim_bufr_records);
    s1sc.define(outncid, dim_bufr_records);
    s1usn.define(outncid, dim_bufr_records);
    s1cat.define(outncid, dim_bufr_records);
    s1subcat.define(outncid, dim_bufr_records);
    s1localsubcat.define(outncid, dim_bufr_records);
    s1mtv.define(outncid, dim_bufr_records);
    s1ltv.define(outncid, dim_bufr_records);
    s1date.define(outncid, dim_bufr_records);
    s1time.define(outncid, dim_bufr_records);
    sec1.define(outncid, dim_bufr_records);
    sec2.define(outncid, dim_bufr_records);
    arrays.define(outncid, dim_bufr_records);

    // TODO nc_put_att       /* put attribute: assign attribute values */

    // End define mode
    res = nc_enddef(outncid);
    error_netcdf::throwf_iferror(res, "leaving define mode for file %s", "##TODO##");

    edition.putvar(outncid);
    s1mtn.putvar(outncid);
    s1ce.putvar(outncid);
    s1sc.putvar(outncid);
    s1usn.putvar(outncid);
    s1cat.putvar(outncid);
    s1subcat.putvar(outncid);
    s1localsubcat.putvar(outncid);
    s1mtv.putvar(outncid);
    s1ltv.putvar(outncid);
    s1date.putvar(outncid);
    s1time.putvar(outncid);
    sec1.putvar(outncid);
    sec2.putvar(outncid);
    arrays.putvar(outncid);
}

}
