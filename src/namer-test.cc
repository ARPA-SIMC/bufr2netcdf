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

#include "namer.h"
#include "options.h"
#include <tests/tests.h>

using namespace b2nc;
using namespace wreport;
using namespace wreport::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("no_rep", []() {
            // No repetition counts
            Options opts;
            opts.use_mnemonic = false;
            unique_ptr<Namer> n(Namer::get(opts));
            string name, mnemo;

            size_t tag = 0;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_000");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_000");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_001");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 3), tag, name, mnemo);
            wassert(actual(name) == "Data_B01003_000");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_001");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_002");
        });

        add_method("one_rep", []() {
            // One simple repetition count
            Options opts;
            opts.use_mnemonic = false;
            unique_ptr<Namer> n(Namer::get(opts));
            string name, mnemo;

            size_t tag = 0;

            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_000");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_001");

            tag = 1;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_002");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_003");

            tag = 0;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_004");
        });

        add_method("nested_rep", []() {
            // Multiple nested repetition counts
            Options opts;
            opts.use_mnemonic = false;
            unique_ptr<Namer> n(Namer::get(opts));
            string name, mnemo;

            size_t tag = 0;

            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_000");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_000");

            tag = 1;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_001");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_001");

            tag = 0;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_002");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_002");

            tag = 2;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_003");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_003");

            tag = 3;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_004");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_004");

            tag = 0;
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 1), tag, name, mnemo);
            wassert(actual(name) == "Data_B01001_005");
            n->name(Namer::DT_DATA, WR_VAR(0, 1, 2), tag, name, mnemo);
            wassert(actual(name) == "Data_B01002_005");
        });
    }
} tests("namer");

}
