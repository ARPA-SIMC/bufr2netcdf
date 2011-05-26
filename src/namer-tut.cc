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
#include <tests/tests.h>
#include <wibble/string.h>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct namer_shar
{
    namer_shar()
    {
    }

    ~namer_shar()
    {
    }
};
TESTGRP(namer);

// No repetition counts
template<> template<>
void to::test<1>()
{
    auto_ptr<Namer> n(Namer::get(Namer::PLAIN));

    string tag = "";

    for (int i = 0; i < 3; ++i)
    {
        try {
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_000");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_000");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_001");
            ensure_equals(n->name(WR_VAR(0, 1, 3), tag), "TODO_B01003_000");
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_001");
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_002");
        } catch (tut::failure& e) {
            throw tut::failure(str::fmtf("%s (iteration %i)", e.what(), i));
        }
    }
}

// One simple repetition count
template<> template<>
void to::test<2>()
{
    auto_ptr<Namer> n(Namer::get(Namer::PLAIN));

    for (int i = 0; i < 3; ++i)
    {
        try {
            string tag = "";

            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_000");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_001");

            tag = "1";
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_002");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_003");
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_002");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_003");

            tag = "";
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_004");
        } catch (tut::failure& e) {
            throw tut::failure(str::fmtf("%s (iteration %i)", e.what(), i));
        }
    }
}

// Multiple nested repetition counts
template<> template<>
void to::test<3>()
{
    auto_ptr<Namer> n(Namer::get(Namer::PLAIN));

    for (int i = 0; i < 3; ++i)
    {
        try {
            string tag = "";

            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_000");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_000");

            tag = "1";
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_001");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_001");
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_001");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_001");

            tag = "";
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_002");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_002");

            tag = "2";
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_003");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_003");
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_003");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_003");

            tag = "2_1";
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_004");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_004");
            n->start(tag);
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_004");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_004");

            tag = "";
            ensure_equals(n->name(WR_VAR(0, 1, 1), tag), "TODO_B01001_005");
            ensure_equals(n->name(WR_VAR(0, 1, 2), tag), "TODO_B01002_005");
        } catch (tut::failure& e) {
            throw tut::failure(str::fmtf("%s (iteration %i)", e.what(), i));
        }
    }
}

}

// vim:set ts=4 sw=4:
