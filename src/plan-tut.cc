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

#include "plan.h"
#include "options.h"
#include "valarray.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wreport/bulletin.h>
#include <cstdio>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct plan_shar
{
    plan_shar()
    {
    }

    ~plan_shar()
    {
    }
};
TESTGRP(plan);

static void read_nth_bufr(BufrBulletin& bulletin, const std::string& testname, unsigned idx=0)
{
    // Open input file
    string srcfile(b2nc::tests::datafile("bufr/" + testname));
    FILE* infd = fopen(srcfile.c_str(), "rb");
    if (infd == NULL)
        error_system::throwf("cannot open %s", srcfile.c_str());

    string rawmsg;
    for (unsigned i = 0; i <= idx && BufrBulletin::read(infd, rawmsg, srcfile.c_str()); ++i)
    {
        // Decode the BUFR message
        bulletin.decode(rawmsg);
    }

    fclose(infd);
}

template<> template<>
void to::test<1>()
{
    Options opts;

    auto_ptr<BufrBulletin> bulletin(BufrBulletin::create());
    read_nth_bufr(*bulletin, "cdfin_acars");

    Plan plan(opts);
    plan.build(*bulletin);

    wassert(actual(plan.sections.size()) == 2);

    // Check toplevel section
    const plan::Section* s = plan.sections[0];
    wassert(actual(s->entries.size()) == 41);

    // All entries except #26 have data
    for (unsigned i = 0; i < 41; ++i)
    {
        if (i == 26) continue;
        WIBBLE_TEST_INFO(info);
        info() << "Entry " << i << "/41";
        wassert(actual(s->entries[i]->data).istrue());
        wassert(actual(s->entries[i]->qbits).isfalse());
    }

    // Check plan contents
    wassert(actual(s->entries[ 0]->data->name) == "MMIOGC");
    wassert(actual(s->entries[ 1]->data->name) == "MMIOGS");
    wassert(actual(s->entries[ 2]->data->name) == "YAIRN");
    wassert(actual(s->entries[ 3]->data->name) == "NOSNO");
    wassert(actual(s->entries[ 4]->data->name) == "MLAH");
    wassert(actual(s->entries[ 5]->data->name) == "MLOH");
    wassert(actual(s->entries[ 6]->data->name) == "MJJJ");
    wassert(actual(s->entries[ 7]->data->name) == "MMM");
    wassert(actual(s->entries[ 8]->data->name) == "MYY");
    wassert(actual(s->entries[ 9]->data->name) == "MGG");
    wassert(actual(s->entries[10]->data->name) == "NGG");
    wassert(actual(s->entries[11]->data->name) == "MSEC");
    wassert(actual(s->entries[12]->data->name) == "NFLEV");
    wassert(actual(s->entries[13]->data->name) == "NDEPF");
    wassert(actual(s->entries[14]->data->name) == "NDNDN");
    wassert(actual(s->entries[15]->data->name) == "NFNFN");
    wassert(actual(s->entries[16]->data->name) == "MB");
    wassert(actual(s->entries[17]->data->name) == "NMDEWX");
    wassert(actual(s->entries[18]->data->name) == "MTDBT");
    wassert(actual(s->entries[19]->data->name) == "MAIV");
    wassert(actual(s->entries[20]->data->name) == "MPHAI");
    wassert(actual(s->entries[21]->data->name) == "MQARA");
    wassert(actual(s->entries[22]->data->name) == "MUUU");
    wassert(actual(s->entries[23]->data->name) == "MTDNH");
    wassert(actual(s->entries[24]->data->name) == "MMIXR");
    // Delayed replication count goes in containing section
    wassert(actual(s->entries[25]->data->name) == "MDREP");
    // Replicated sections are represented by a placeholder
    wassert(actual(s->entries[26]->data).isfalse());
    wassert(actual(s->entries[26]->subsection).istrue());
    wassert(actual(s->entries[26]->subsection->id) == 1);
    // The containing section continues fine after the replicated section
    wassert(actual(s->entries[27]->data->name) == "MTUIN");
    wassert(actual(s->entries[28]->data->name) == "NTIED");
    wassert(actual(s->entries[29]->data->name) == "NRED");
    wassert(actual(s->entries[30]->data->name) == "NAICE");
    wassert(actual(s->entries[31]->data->name) == "NPLWC");
    wassert(actual(s->entries[32]->data->name) == "NALWC");
    wassert(actual(s->entries[33]->data->name) == "NSLD");
    wassert(actual(s->entries[34]->data->name) == "MAICI");
    wassert(actual(s->entries[35]->data->name) == "MPOTO");
    wassert(actual(s->entries[36]->data->name) == "NADRS");
    wassert(actual(s->entries[37]->data->name) == "NOSLL");
    wassert(actual(s->entries[38]->data->name) == "YAGRS");
    wassert(actual(s->entries[39]->data->name) == "MPN");
    wassert(actual(s->entries[40]->data->name) == "MMRQ");

    // Check replicated section
    s = plan.sections[1];
    wassert(actual(s->entries.size()) == 2);

    // Normal variables are there, even if the section might be repeated 0
    // times
    wassert(actual(s->entries[0]->data).istrue());
    wassert(actual(s->entries[0]->data->name) == "MMTI");
    wassert(actual(s->entries[1]->data).istrue());
    wassert(actual(s->entries[1]->data->name) == "MPTI");

    //plan.print(stderr);
}

template<> template<>
void to::test<2>()
{
    Options opts;

    auto_ptr<BufrBulletin> bulletin(BufrBulletin::create());
    read_nth_bufr(*bulletin, "cdfin_gps_zenith");

    Plan plan(opts);
    plan.build(*bulletin);

    wassert(actual(plan.sections.size()) == 2);

    // Check toplevel section
    const plan::Section* s = plan.sections[0];
    wassert(actual(s->entries.size()) == 26);

    // All entries except #16 have data
    for (unsigned i = 0; i < 26; ++i)
    {
        if (i == 16) continue;
        WIBBLE_TEST_INFO(info);
        info() << "Entry " << i << "/26";
        wassert(actual(s->entries[i]->data).istrue());
        wassert(actual(s->entries[i]->qbits).isfalse());
    }

    // Check plan contents
    wassert(actual(s->entries[ 0]->data->name) == "YSOSN");
    wassert(actual(s->entries[ 1]->data->name) == "MJJJ");
    wassert(actual(s->entries[ 2]->data->name) == "MMM");
    wassert(actual(s->entries[ 3]->data->name) == "MYY");
    wassert(actual(s->entries[ 4]->data->name) == "MGG");
    wassert(actual(s->entries[ 5]->data->name) == "NGG");
    wassert(actual(s->entries[ 6]->data->name) == "MLAH");
    wassert(actual(s->entries[ 7]->data->name) == "MLOH");
    wassert(actual(s->entries[ 8]->data->name) == "MHP");
    wassert(actual(s->entries[ 9]->data->name) == "MTISI");
    wassert(actual(s->entries[10]->data->name) == "NGGTP");
    wassert(actual(s->entries[11]->data->name) == "MPPP");
    wassert(actual(s->entries[12]->data->name) == "MTN");
    wassert(actual(s->entries[13]->data->name) == "MUUU");
    wassert(actual(s->entries[14]->data->name) == "NQFGD");
    wassert(actual(s->entries[15]->data->name) == "MTOTN");
    // Replicated sections are represented by a placeholder
    wassert(actual(s->entries[16]->data).isfalse());
    wassert(actual(s->entries[16]->subsection).istrue());
    wassert(actual(s->entries[16]->subsection->id) == 1);
    wassert(actual(s->entries[17]->data->name) == "MSSMS");
    wassert(actual(s->entries[18]->data->name) == "NDPDL");
    wassert(actual(s->entries[19]->data->name) == "NEEPDD");
    wassert(actual(s->entries[20]->data->name) == "MSSMS0");
    wassert(actual(s->entries[21]->data->name) == "NDPDL0");
    wassert(actual(s->entries[22]->data->name) == "NEEPDD0");
    wassert(actual(s->entries[23]->data->name) == "NCZWV");
    wassert(actual(s->entries[24]->data->name) == "NWLN");
    wassert(actual(s->entries[25]->data->name) == "MLIED");

    // Check replicated section
    s = plan.sections[1];
    wassert(actual(s->entries.size()) == 6);

    // All entries have data
    for (unsigned i = 0; i < 6; ++i)
    {
        WIBBLE_TEST_INFO(info);
        info() << "Entry " << i << "/6";
        wassert(actual(s->entries[i]->data).istrue());
    }
    // Check plan contents
    wassert(actual(s->entries[ 0]->data->name) == "MSACL");
    wassert(actual(s->entries[ 1]->data->name) == "MPTID");
    wassert(actual(s->entries[ 2]->data->name) == "MDA");
    wassert(actual(s->entries[ 3]->data->name) == "MDE");
    wassert(actual(s->entries[ 4]->data->name) == "NADES");
    wassert(actual(s->entries[ 5]->data->name) == "NEERR");

    //plan.print(stderr);
}

template<> template<>
void to::test<3>()
{
    Options opts;

    auto_ptr<BufrBulletin> bulletin(BufrBulletin::create());
    read_nth_bufr(*bulletin, "cdfin_buoy");

    Plan plan(opts);
    plan.build(*bulletin);

    wassert(actual(plan.sections.size()) == 5);

    // Check toplevel section
    const plan::Section* s = plan.sections[0];
    wassert(actual(s->entries.size()) == 106);

    // All entries except replicated sections have data
    for (unsigned i = 0; i < 106; ++i)
    {
        WIBBLE_TEST_INFO(info);
        info() << "Entry " << i << "/106";

        if (i != 61 && i != 65 && i != 103 && i != 105)
            wassert(actual(s->entries[i]->data).istrue());
        else
            wassert(actual(s->entries[i]->data).isfalse());

        if (i != 0 && i != 60 && i != 61 && i != 64 && i != 65 && i != 103 && i != 104 && i != 105)
            wassert(actual(s->entries[i]->qbits).istrue());
        else
            wassert(actual(s->entries[i]->qbits).isfalse());
    }

    // Check plan contents
    wassert(actual(s->entries[  0]->data->name) == "MADDF");
    wassert(actual(s->entries[  1]->data->name) == "MA1");
    wassert(actual(s->entries[  2]->data->name) == "MWRSA");
    wassert(actual(s->entries[  3]->data->name) == "MABNN");
    wassert(actual(s->entries[  4]->data->name) == "NIX");
    wassert(actual(s->entries[  5]->data->name) == "NBOTY");
    wassert(actual(s->entries[  6]->data->name) == "MTODB");
    wassert(actual(s->entries[  7]->data->name) == "MJJJ");
    wassert(actual(s->entries[  8]->data->name) == "MMM");
    wassert(actual(s->entries[  9]->data->name) == "MYY");
    wassert(actual(s->entries[ 10]->data->name) == "MGG");
    wassert(actual(s->entries[ 11]->data->name) == "NGG");
    wassert(actual(s->entries[ 12]->data->name) == "MTISI");
    wassert(actual(s->entries[ 13]->data->name) == "MJJJ0");
    wassert(actual(s->entries[ 14]->data->name) == "MMM0");
    wassert(actual(s->entries[ 15]->data->name) == "MYY0");
    wassert(actual(s->entries[ 16]->data->name) == "MGG0");
    wassert(actual(s->entries[ 17]->data->name) == "NGG0");
    wassert(actual(s->entries[ 18]->data->name) == "MTISI0");
    wassert(actual(s->entries[ 19]->data->name) == "MLAH");
    wassert(actual(s->entries[ 20]->data->name) == "MLOH");
    wassert(actual(s->entries[ 21]->data->name) == "NALAH");
    wassert(actual(s->entries[ 22]->data->name) == "NALOH");
    wassert(actual(s->entries[ 23]->data->name) == "MHOSNN");
    wassert(actual(s->entries[ 24]->data->name) == "YPTID");
    wassert(actual(s->entries[ 25]->data->name) == "MDCS");
    wassert(actual(s->entries[ 26]->data->name) == "MDS");
    wassert(actual(s->entries[ 27]->data->name) == "MDSDS");
    wassert(actual(s->entries[ 28]->data->name) == "MMRVMT");
    wassert(actual(s->entries[ 29]->data->name) == "MQBST");
    wassert(actual(s->entries[ 30]->data->name) == "MQOBL");
    wassert(actual(s->entries[ 31]->data->name) == "MLQC");
    wassert(actual(s->entries[ 32]->data->name) == "NZD");
    wassert(actual(s->entries[ 33]->data->name) == "NDW");
    wassert(actual(s->entries[ 34]->data->name) == "MPW");
    wassert(actual(s->entries[ 35]->data->name) == "MHW");
    wassert(actual(s->entries[ 36]->data->name) == "NDWDW");
    wassert(actual(s->entries[ 37]->data->name) == "MPWPW");
    wassert(actual(s->entries[ 38]->data->name) == "MHWHW");
    wassert(actual(s->entries[ 39]->data->name) == "NDW12");
    wassert(actual(s->entries[ 40]->data->name) == "MPW12");
    wassert(actual(s->entries[ 41]->data->name) == "MHW12");
    wassert(actual(s->entries[ 42]->data->name) == "MTYOE");
    wassert(actual(s->entries[ 43]->data->name) == "NBVLR");
    wassert(actual(s->entries[ 44]->data->name) == "MTYOE0");
    wassert(actual(s->entries[ 45]->data->name) == "NBVLR0");
    wassert(actual(s->entries[ 46]->data->name) == "MTYOE1");
    wassert(actual(s->entries[ 47]->data->name) == "NBVLR1");
    wassert(actual(s->entries[ 48]->data->name) == "MTYOE2");
    wassert(actual(s->entries[ 49]->data->name) == "NDRTY");
    wassert(actual(s->entries[ 50]->data->name) == "MLDDS");
    wassert(actual(s->entries[ 51]->data->name) == "NDRDE");
    wassert(actual(s->entries[ 52]->data->name) == "NLDS");
    wassert(actual(s->entries[ 53]->data->name) == "MDCI");
    wassert(actual(s->entries[ 54]->data->name) == "NCABL");
    wassert(actual(s->entries[ 55]->data->name) == "MHPEC");
    wassert(actual(s->entries[ 56]->data->name) == "MSI");
    wassert(actual(s->entries[ 57]->data->name) == "MMOSTM");
    wassert(actual(s->entries[ 58]->data->name) == "NK1");
    wassert(actual(s->entries[ 59]->data->name) == "NK2");
    wassert(actual(s->entries[ 60]->data->name) == "MDREP");
    wassert(actual(s->entries[ 61]->data).isfalse());
    wassert(actual(s->entries[ 61]->subsection).istrue());
    wassert(actual(s->entries[ 61]->subsection->id) == 1);
    wassert(actual(s->entries[ 62]->data->name) == "MMOCM");
    wassert(actual(s->entries[ 63]->data->name) == "NK3K4");
    wassert(actual(s->entries[ 64]->data->name) == "MDREP0");
    wassert(actual(s->entries[ 65]->data).isfalse());
    wassert(actual(s->entries[ 65]->subsection).istrue());
    wassert(actual(s->entries[ 65]->subsection->id) == 2);
    wassert(actual(s->entries[ 66]->data->name) == "MHOBNN");
    wassert(actual(s->entries[ 67]->data->name) == "MTYOE3");
    wassert(actual(s->entries[ 68]->data->name) == "MTINST");
    wassert(actual(s->entries[ 69]->data->name) == "MPPP");
    wassert(actual(s->entries[ 70]->data->name) == "MPPPP");
    wassert(actual(s->entries[ 71]->data->name) == "NPPP");
    wassert(actual(s->entries[ 72]->data->name) == "NA");
    wassert(actual(s->entries[ 73]->data->name) == "MTYOE4");
    wassert(actual(s->entries[ 74]->data->name) == "MHOSEN");
    wassert(actual(s->entries[ 75]->data->name) == "MHAWAS");
    wassert(actual(s->entries[ 76]->data->name) == "MTDBT");
    wassert(actual(s->entries[ 77]->data->name) == "MTDNH");
    wassert(actual(s->entries[ 78]->data->name) == "MUUU");
    wassert(actual(s->entries[ 79]->data->name) == "MHOSEN0");
    wassert(actual(s->entries[ 80]->data->name) == "MHAWAS0");
    wassert(actual(s->entries[ 81]->data->name) == "NACH2V");
    wassert(actual(s->entries[ 82]->data->name) == "MHAWAS1");
    wassert(actual(s->entries[ 83]->data->name) == "MANTYP");
    wassert(actual(s->entries[ 84]->data->name) == "NIW");
    wassert(actual(s->entries[ 85]->data->name) == "MTISI1");
    wassert(actual(s->entries[ 86]->data->name) == "NGGTP");
    wassert(actual(s->entries[ 87]->data->name) == "NDNDN");
    wassert(actual(s->entries[ 88]->data->name) == "NFNFN");
    wassert(actual(s->entries[ 89]->data->name) == "MTISI2");
    wassert(actual(s->entries[ 90]->data->name) == "NGGTP0");
    wassert(actual(s->entries[ 91]->data->name) == "NMWGD");
    wassert(actual(s->entries[ 92]->data->name) == "NFXGU");
    wassert(actual(s->entries[ 93]->data->name) == "NACH2V0");
    wassert(actual(s->entries[ 94]->data->name) == "MHAWAS2");
    wassert(actual(s->entries[ 95]->data->name) == "MHOSEN1");
    wassert(actual(s->entries[ 96]->data->name) == "MGGTP");
    wassert(actual(s->entries[ 97]->data->name) == "MRRR");
    wassert(actual(s->entries[ 98]->data->name) == "MHOSEN2");
    wassert(actual(s->entries[ 99]->data->name) == "MTISI3");
    wassert(actual(s->entries[100]->data->name) == "MGGTP0");
    wassert(actual(s->entries[101]->data->name) == "MSSSS");
    wassert(actual(s->entries[102]->data->name) == "MTISI4");
    wassert(actual(s->entries[103]->data).isfalse());
    wassert(actual(s->entries[103]->subsection).istrue());
    wassert(actual(s->entries[103]->subsection->id) == 3);
    wassert(actual(s->entries[104]->data->name) == "MDREP1");
    wassert(actual(s->entries[105]->data).isfalse());
    wassert(actual(s->entries[105]->subsection).istrue());
    wassert(actual(s->entries[105]->subsection->id) == 4);

    // Check other sections
    s = plan.sections[1];
    wassert(actual(s->entries.size()) == 3);
    wassert(actual(s->entries[0]->data).istrue());
    wassert(actual(s->entries[1]->data).istrue());
    wassert(actual(s->entries[2]->data).istrue());
    wassert(actual(s->entries[0]->data->name) == "NZNZN");
    wassert(actual(s->entries[1]->data->name) == "MTN00");
    wassert(actual(s->entries[2]->data->name) == "MSNSN");

    s = plan.sections[2];
    wassert(actual(s->entries.size()) == 3);
    wassert(actual(s->entries[0]->data).istrue());
    wassert(actual(s->entries[1]->data).istrue());
    wassert(actual(s->entries[2]->data).istrue());
    wassert(actual(s->entries[0]->qbits).istrue());
    wassert(actual(s->entries[1]->qbits).istrue());
    wassert(actual(s->entries[2]->qbits).istrue());
    wassert(actual(s->entries[0]->data->name) == "NZNZN0");
    wassert(actual(s->entries[1]->data->name) == "NDCDC");
    wassert(actual(s->entries[2]->data->name) == "NFCFC");

    s = plan.sections[3];
    wassert(actual(s->entries.size()) == 1);
    wassert(actual(s->entries[0]->data).istrue());
    wassert(actual(s->entries[0]->qbits).istrue());
    wassert(actual(s->entries[0]->data->name) == "NOMDP");

    s = plan.sections[4];
    wassert(actual(s->entries.size()) == 1);
    wassert(actual(s->entries[0]->data).istrue());
    wassert(actual(s->entries[0]->qbits).isfalse());
    wassert(actual(s->entries[0]->data->name) == "YSUPL");

    //plan.print(stderr);
}

}

// vim:set ts=4 sw=4:

