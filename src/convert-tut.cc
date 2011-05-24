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
#include <wibble/string.h>
#include <wibble/sys/fs.h>
#include <netcdf.h>
#include <cstdlib>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
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
    string tmpfile;
    FILE* infd;
    int outncid;

    Convtest(const char* testname)
        : srcfile(b2nc::tests::datafile("bufr/cdfin_acars")),
          resfile(b2nc::tests::datafile("netcdf/cdfin_acars")),
          // We are run in a known temp dir, so we can hardcode the temporary
          // file name
          tmpfile("tmpfile.nc"),
          infd(NULL), outncid(-1)
    {
        // Open input file
        infd = fopen(srcfile.c_str(), "rb");
        if (infd == NULL)
            error_system::throwf("cannot open %s", srcfile.c_str());

        // Create output file
        int res = nc_create(tmpfile.c_str(), NC_CLOBBER, &outncid);
        error_netcdf::throwf_iferror(res, "Creating file %s", tmpfile.c_str());
    }

    ~Convtest()
    {
        if (infd != NULL)
            fclose(infd);
        if (outncid != -1)
            nc_close(outncid);
    }

    void convert()
    {
        // Perform the conversion
        Converter conv;
        conv.convert(infd, outncid);

        // Close files
        fclose(infd);
        infd = NULL;
        int res = nc_close(outncid);
        error_netcdf::throwf_iferror(res, "Closing file %s", tmpfile.c_str());
        outncid = -1;

        // Compare results
        const char* nccmp = getenv("B2NC_NCCMP");
        ensure(nccmp);
        ensure(sys::fs::access(nccmp, X_OK));
        string cmd = str::fmtf("%s -d -m -g %s %s 2>&1", nccmp, resfile.c_str(), tmpfile.c_str());
        char line[1024];
        FILE* cmpres = popen(cmd.c_str(), "r");
        if (cmpres == NULL)
            error_system::throwf("opening pipe from \"%s\"", cmd.c_str());
        bool has_errors = false;
        while (fgets(line, 1024, cmpres) != NULL)
        {
            fprintf(stderr, "%s", line);
            has_errors = true;
        }
        if (!feof(cmpres))
            error_system::throwf("reading from \"%s\"", cmd.c_str());
        ensure_equals(pclose(cmpres), 0);
        ensure(!has_errors);
    }
};

template<> template<>
void to::test<1>()
{
    Convtest t("cdfin_acars");
    t.convert();
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
