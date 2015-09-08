/*
 * Copyright (C) 2011--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <wreport/utils/sys.h>
#include <netcdf.h>
#include <unistd.h>
#include <cstdlib>
#include <regex.h>

using namespace b2nc;
using namespace wreport;
using namespace wreport::tests;
using namespace std;

namespace {

struct Regexp
{
    regex_t compiled;

    Regexp(const char* regex)
    {
        if (int err = regcomp(&compiled, regex, REG_EXTENDED | REG_NOSUB))
            raise_error(err);
    }
    ~Regexp()
    {
        regfree(&compiled);
    }

    bool search(const char* s)
    {
        return regexec(&compiled, s, 0, nullptr, 0) != REG_NOMATCH;
    }

    void raise_error(int code)
    {
        // Get the size of the error message string
        size_t size = regerror(code, &compiled, nullptr, 0);

        char* buf = new char[size];
        regerror(code, &compiled, buf, size);
        string msg(buf);
        delete[] buf;
        throw std::runtime_error(msg);
    }
};

struct MultiRegexp
{
    vector<Regexp*> exps;

    ~MultiRegexp()
    {
        for (auto& i: exps)
            delete i;
    }

    void add(const std::string& expr)
    {
        exps.push_back(new Regexp(expr.c_str()));
    }

    bool match(const std::string& s)
    {
        for (auto& i: exps)
            if (i->search(s.c_str()))
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
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : CODE.TABLE <> CODE TABLE( +[0-9]+)?");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : FLAG_TABLE <> FLAG TABLE( +[0-9]+)?");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : CCITT.IA5 <> CCITTIA5");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : DEGREE_TRUE <> DEGREE TRUE");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : PART_PER_THO <> PART PER THOUSAND");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : LOG\\(1/M\\*\\*2\\) <> LOG \\(1/M2\\)");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : 1/S <> Hz");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : DB <> dB");
        ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : units : VALUES : M\\*\\*\\(2/3\\)/S <> m\\*\\*\\(2/3\\)/S");
    }

    void make_netcdf()
    {
        // Create output file
        Options options;
        unique_ptr<Outfile> outfile = Outfile::get(options);
        outfile->open(tmpfile);

        // Convert source file
        read_bufr(srcfile, *outfile);

        // Flush output
        outfile->close();
    }

    void convert()
    {
        make_netcdf();

        // Compare results
        const char* nccmp = getenv("B2NC_NCCMP");
        wassert(actual(nccmp).istrue());
        wassert(actual(sys::access(nccmp, X_OK)).istrue());
        char cmd[1024];
        snprintf(cmd, 1024, "%s -d -m -g -f %s %s 2>&1", nccmp, resfile.c_str(), tmpfile.c_str());
        char line[1024];
        FILE* cmpres = popen(cmd, "r");
        if (cmpres == NULL)
            error_system::throwf("opening pipe from \"%s\"", cmd);

        vector<string> problems;
        while (fgets(line, 1024, cmpres) != NULL)
        {
            if (ignore_list.match(line))
                continue;
            problems.push_back(line);
            fprintf(stderr, "%s", line);
        }
        if (!feof(cmpres))
            error_system::throwf("reading from \"%s\"", cmd);
        wassert(actual(problems.size()) == 0u);
        // nccmp documentation states that its exit code is unreliable
        // also, it might find errors that we are ignoring
        pclose(cmpres); // wassert(actual(pclose(cmpres)) == 0);
    }
};

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("acars", []() {
            Convtest t("cdfin_acars");
            // Subcentre is correctly reported as referencing centre, but not in the original NetCDF files
            t.ignore_list.add("^DIFFER : VARIABLE \"MMIOGS\" IS MISSING ATTRIBUTE WITH NAME \"references\" IN FILE \".+/netcdf/cdfin_acars\"");
            t.convert();
        });

        add_method("acars_uk", []() {
            Convtest t("cdfin_acars_uk");
            t.convert();
        });

        add_method("acars_us", []() {
            Convtest t("cdfin_acars_us");
            // TODO: Looks like quirks in nccmp
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : + <> +\n$");
            // TODO: Variable does not exist in our BUFR tables
            t.ignore_list.add("^DIFFER : NAME : VARIABLE : 011235 : VARIABLE DOESN'T EXIST IN \"tmpfile.nc\"");
            t.convert();
        });

        add_method("amdar", []() {
            Convtest t("cdfin_amdar");
            t.convert();
        });

        add_method("buoy", []() {
            Convtest t("cdfin_buoy");

            // This file was encoded ignoring the fact that MDREP and MDREP1 always have the same value
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP1 <> _constant");

            t.convert();
        });

        add_method("gps_zenith", []() {
            Convtest t("cdfin_gps_zenith");
            // The variable is integer in the WMO tables, no idea why the sample NetCDF has float
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE : NWLN : FLOAT <> INT");
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE \"NWLN\" : FLOAT <> INT");
            t.convert();
        });

        add_method("pilot", []() {
            Convtest t("cdfin_pilot");
            // This file was encoded ignoring the fact that MDREP always has the same value
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
            t.convert();
        });

        add_method("pilot_p", []() {
            Convtest t("cdfin_pilot_p");
            // This file was encoded ignoring the fact that MDREP always has the same value
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
            t.convert();
        });

        add_method("radar_vad", []() {
            Convtest t("cdfin_radar_vad");
            t.convert();
        });

        add_method("rass", []() {
            Convtest t("cdfin_rass");
            // The file was encoded assuming than with B31021=1 and a qbit value of 1
            // the quality information is missing, but CODE TABLE 031021 says that 1
            // means "suspect or bad"
            t.ignore_list.add("^DIFFER : VARIABLE : MWMPSQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 1");
            t.ignore_list.add("^DIFFER : VARIABLE : MTVIRQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 1");

            // We render values scaled by positive powers if 10 as floats, not ints
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE : MRGLEN : INT <> FLOAT");
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE \"MRGLEN\" : INT <> FLOAT");

            // TODO: Looks like a quirk in nccmp
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> -2147483647");
            // TODO: Another case of bufrx2netcdf skipping some references
            t.ignore_list.add("^DIFFER : VARIABLE \"MMIOGS\" IS MISSING ATTRIBUTE WITH NAME \"references\" IN FILE \".+/netcdf/cdfin_rass\"");
            t.convert();
        });

        add_method("ship", []() {
            Convtest t("cdfin_ship");
            // TODO: In the sample data, multiple references to MGGTP look swapped
            t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference_004024 : VALUES : MGGTP2 <> MGGTP1");
            t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference2_004024 : VALUES : MGGTP1 <> MGGTP2");
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference_004024 : VALUES : MGGTP4 <> MGGTP3");
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference2_004024 : VALUES : MGGTP3 <> MGGTP4");

            t.convert();
        });

        add_method("synop", []() {
            Convtest t("cdfin_synop");

            // WMO block number is correctly reported as referencing Short station or site name, but not in the original NetCDF files
            t.ignore_list.add("^DIFFER : VARIABLE \"MII\" IS MISSING ATTRIBUTE WITH NAME \"references\" IN FILE \".+/netcdf/cdfin_synop\"");

            t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference_004024 : VALUES : MGGTP3 <> MGGTP2");
            t.ignore_list.add("^DIFFER : VARIABLE : MTXTXH : ATTRIBUTE : reference2_004024 : VALUES : MGGTP2 <> MGGTP3");
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference_004024 : VALUES : MGGTP5 <> MGGTP4");
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : reference2_004024 : VALUES : MGGTP4 <> MGGTP5");

            // We render values scaled by positive powers if 10 as floats, not ints
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE : MRGLEN : INT <> FLOAT");
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE \"MRGLEN\" : INT <> FLOAT");

            t.convert();
        });

        add_method("temp", []() {
            Convtest t("cdfin_temp");
            // MDREP is constant in this case
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
            t.convert();
        });

        add_method("tempship", []() {
            Convtest t("cdfin_tempship");
            // MDREP is constant in this case
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : ATTRIBUTE : dim1_length : VALUES : MDREP <> _constant");
            t.convert();
        });

        add_method("wprof", []() {
            Convtest t("cdfin_wprof");

            // Subcentre is correctly reported as referencing centre, but not in the original NetCDF files
            t.ignore_list.add("^DIFFER : VARIABLE \"MMIOGS\" IS MISSING ATTRIBUTE WITH NAME \"references\" IN FILE \".+/netcdf/cdfin_wprof\"");

            // TODO: Looks like quirks in nccmp
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> -2147483647");
            t.ignore_list.add("^DIFFER : VARIABLE : [A-Z0-9]+ : POSITION : [0-9]+ [0-9]+ : VALUES : 0 <> 0\n$");

            // The file was encoded assuming than with B31021=1 and a qbit value of 1
            // the quality information is missing, but CODE TABLE 031021 says that 1
            // means "suspect or bad"
            t.ignore_list.add("^DIFFER : VARIABLE : MWMPSQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 1");
            t.ignore_list.add("^DIFFER : VARIABLE : NDNDNQ : POSITION : [0-9]+ [0-9]+ : VALUES : -2147483647 <> 1");

            // We render values scaled by positive powers if 10 as floats, not ints
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE : MRGLEN : INT <> FLOAT");
            t.ignore_list.add("^DIFFER : TYPE.+ VARIABLE \"MRGLEN\" : INT <> FLOAT");

            t.convert();
        });

        add_method("bug_temp", []() {
            Convtest t("bug_temp");
            t.make_netcdf();
        });

        add_method("amsua", []() {
            Convtest t("AMSUA.bufr");
            t.make_netcdf();
        });

        add_method("atms", []() {
            Convtest t("atms2.bufr");
            t.make_netcdf();
        });
    }
} tests("convert");

}
