/*
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
#include "utils.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <netcdf.h>

using namespace wreport;
using namespace b2nc;
using namespace std;

namespace tut {

struct convert_shar
{
    convert_shar()
    {
    }

    ~convert_shar()
    {
    }
};
TESTGRP(convert);

struct Convtest
{
    string srcfile;
    string resfile;
    FILE* infd;
    int outncid;

    Convtest(const char* testname)
        srcfile(tests::datafile("bufr/cdfin_acars")),
        resfile(tests::datafile("netcdf/cdfin_acars")),
        infd(NULL), outncid(-1)
    {
        // Open input file
        infd = fopen(srcfile.c_str(), "rb");
        if (infd == NULL)
            error_system::throwf("cannot open %s", srcfile.c_str());

        // Create output file
        int res = nc_create("tmpfile.nc", NC_CLOBBER, &outncid);
        error_netcdf::throwf_iferror(res, "Creating file tmpfile.nc");
    }

    ~Convtest()
    {
        if (infd != NULL)
            fclose(infd);
    }
};

template<> template<>
void to::test<1>()
{
    Convtest t("cdfin_acars");

    Converter conv;

    conv.convert(t.infd, NcFile& out);
}

// TODO   cdfin_acars_uk
// TODO   cdfin_acars_us
// TODO   cdfin_amdar
// TODO   cdfin_buoy
// TODO   cdfin_gps_zenith
// TODO   cdfin_pilot
// TODO   cdfin_pilot_p
// TODO   cdfin_radar_vad
// TODO   cdfin_rass
// TODO   cdfin_ship
// TODO   cdfin_synop
// TODO   cdfin_temp
// TODO   cdfin_tempship
// TODO   cdfin_wprof

}

// vim:set ts=4 sw=4:
