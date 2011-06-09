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
#include "options.h"
#include "utils.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wibble/regexp.h>
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

namespace {

struct MultiRegexp
{
    vector<ERegexp*> exps;

    ~MultiRegexp()
    {
        for (vector<ERegexp*>::iterator i = exps.begin(); i != exps.end(); ++i)
            delete *i;
    }

    void add(const std::string& expr)
    {
        exps.push_back(new ERegexp(expr));
    }

    bool match(const std::string& s)
    {
        for (vector<ERegexp*>::const_iterator i = exps.begin(); i != exps.end(); ++i)
            if ((*i)->match(s))
                return true;
        return false;
    }
};

struct Convtest
{
    string srcfile;
    string resfile;
    string tmpfile;
    MultiRegexp ignore_list;

    Convtest(const std::string& testname)
        : srcfile(b2nc::tests::datafile("bufr/" + testname)),
          resfile(b2nc::tests::datafile("netcdf/" + testname)),
          // We are run in a known temp dir, so we can hardcode the temporary
          // file name
          tmpfile("tmpfile.nc")
    {
        // TODO: support it
        ignore_list.add("^DIFFER : NAME : VARIABLE : section2_[a-z_]+ : VARIABLE DOESN'T EXIST IN ");
        // TODO: find out why some reference attributes are missing
        ignore_list.add("^DIFFER : NUMBER OF ATTRIBUTES : VARIABLE : edition_number : [0-9]+ <> 1");
        ignore_list.add("^DIFFER : VARIABLE \"edition_number\" IS MISSING ATTRIBUTE WITH NAME \"reference");
        ignore_list.add("^DIFFER : VARIABLE \"[A-Z0-9]+\" IS MISSING ATTRIBUTE WITH NAME \"reference_[0-9]+\" IN FILE \".+/test/netcdf/cdfin_");
        ignore_list.add("^DIFFER : NUMBER OF ATTRIBUTES : VARIABLE : [A-Z0-9]+ : [0-9]+ <> [0-9]+"); // dangerous but no other way
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : references : VALUES : [0-9,]+ <> [0-9,]+"); // dangerous but no other way
        // TODO: see if importing WMO tables from XML brings matching long names
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : long_name : VALUES : ");

        // Uncontroversial
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : CODE.TABLE <> CODE TABLE [0-9]+");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : FLAG_TABLE <> FLAG TABLE [0-9]+");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : CCITT.IA5 <> CCITTIA5");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : DEGREE_TRUE <> DEGREE TRUE");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : PART_PER_THO <> PART PER THOUSAND");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : LOG\\(1/M\\*\\*2\\) <> LOG \\(1/M2\\)");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : 1/S <> Hz");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : DB <> dB");
    }

    void convert()
    {
        // Create output file
        Options options;
        auto_ptr<Outfile> outfile = Outfile::get(options);
        outfile->open(tmpfile);

        // Convert source file
        read_bufr(srcfile, *outfile);

        // Flush output
        outfile->close();

        // Compare results
        const char* nccmp = getenv("B2NC_NCCMP");
        ensure(nccmp);
        ensure(sys::fs::access(nccmp, X_OK));
        string cmd = str::fmtf("%s -d -m -g -f %s %s 2>&1", nccmp, resfile.c_str(), tmpfile.c_str());
        char line[1024];
        FILE* cmpres = popen(cmd.c_str(), "r");
        if (cmpres == NULL)
            error_system::throwf("opening pipe from \"%s\"", cmd.c_str());

        vector<string> problems;
        while (fgets(line, 1024, cmpres) != NULL)
        {
            if (ignore_list.match(line))
                continue;
            problems.push_back(line);
            fprintf(stderr, "%s", line);
        }
        if (!feof(cmpres))
            error_system::throwf("reading from \"%s\"", cmd.c_str());
        ensure_equals(problems.size(), 0u);
        // nccmp documentation states that its exit code is unreliable
        // also, it might find errors that we are ignoring
        pclose(cmpres); // ensure_equals(pclose(cmpres), 0);
    }
};

}

template<> template<>
void to::test<1>()
{
    Convtest t("cdfin_acars");
    t.ignore_list.add("^DIFFER : VARIABLE \"[A-Z0-9]+\" IS MISSING ATTRIBUTE WITH NAME \"references\" IN FILE \".+/netcdf/cdfin_acars\"");
    t.convert();
}

template<> template<>
void to::test<2>()
{
    Convtest t("cdfin_acars_uk");
    t.convert();
}

template<> template<>
void to::test<3>()
{
    Convtest t("cdfin_acars_us");
    // TODO: Looks like quirks in nccmp
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : + <> +\n$");
    // TODO: Variable does not exist in our BUFR tables
    t.ignore_list.add("^DIFFER : NAME : VARIABLE : 011235 : VARIABLE DOESN'T EXIST IN \"tmpfile.nc\"");
    t.convert();
}

template<> template<>
void to::test<4>()
{
    Convtest t("cdfin_amdar");
    t.convert();
}

template<> template<>
void to::test<5>()
{
    Convtest t("cdfin_buoy");

    // This file was encoded ignoring the fact that MDREP and MDREP1 always have the same value
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP1 <> _constant");

    t.convert();
}

template<> template<>
void to::test<6>()
{
    Convtest t("cdfin_gps_zenith");
    t.convert();
}

template<> template<>
void to::test<7>()
{
    Convtest t("cdfin_pilot");
    // This file was encoded ignoring the fact that MDREP always has the same value
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
    t.convert();
}

template<> template<>
void to::test<8>()
{
    Convtest t("cdfin_pilot_p");
    // This file was encoded ignoring the fact that MDREP always has the same value
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
    t.convert();
}

template<> template<>
void to::test<9>()
{
    Convtest t("cdfin_radar_vad");
    t.convert();
}

template<> template<>
void to::test<10>()
{
    Convtest t("cdfin_rass");
    // The file was encoded assuming than with B31021=1 and a qbit value of 1
    // the quality information is missing, but CODE TABLE 031021 says that 1
    // means "suspect or bad"
    t.ignore_list.add("^DIFFER : VARIABLE : MWMPSQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 3");
    t.ignore_list.add("^DIFFER : VARIABLE : MTVIRQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 3");
    // TODO: Looks like a quirk in nccmp
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> -2147483647");
    // TODO: Another case of bufrx2netcdf skipping some references
    t.ignore_list.add("^DIFFER : VARIABLE \"MMIOGS\" IS MISSING ATTRIBUTE WITH NAME \"references\" IN FILE \".+/netcdf/cdfin_rass\"");
    t.convert();
}

template<> template<>
void to::test<11>()
{
    Convtest t("cdfin_ship");
    // TODO: In the sample data, multiple references to MGGTP look swapped
    t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference_004024 : VALUES : MGGTP2 <> MGGTP1");
    t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference2_004024 : VALUES : MGGTP1 <> MGGTP2");
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference_004024 : VALUES : MGGTP4 <> MGGTP3");
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference2_004024 : VALUES : MGGTP3 <> MGGTP4");

    t.convert();
}

template<> template<>
void to::test<12>()
{
    Convtest t("cdfin_synop");
    t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference_004024 : VALUES : MGGTP3 <> MGGTP2");
    t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference2_004024 : VALUES : MGGTP2 <> MGGTP3");
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference_004024 : VALUES : MGGTP5 <> MGGTP4");
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference2_004024 : VALUES : MGGTP4 <> MGGTP5");
    t.convert();
}

template<> template<>
void to::test<13>()
{
    Convtest t("cdfin_temp");
    // MDREP is constant in this case
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
    t.convert();
}

template<> template<>
void to::test<14>()
{
    Convtest t("cdfin_tempship");
    // MDREP is constant in this case
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
    t.convert();
}

template<> template<>
void to::test<15>()
{
    Convtest t("cdfin_wprof");
    // TODO: Looks like quirks in nccmp
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> -2147483647");
    t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : 0 <> 0\n$");

    // The file was encoded assuming than with B31021=1 and a qbit value of 1
    // the quality information is missing, but CODE TABLE 031021 says that 1
    // means "suspect or bad"
    t.ignore_list.add("^DIFFER : VARIABLE : MWMPSQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 3");
    t.ignore_list.add("^DIFFER : VARIABLE : NDNDNQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 3");

    t.convert();
}

}

// vim:set ts=4 sw=4:
