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

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct namers_shar
{
    namers_shar()
    {
    }

    ~namers_shar()
    {
    }
};
TESTGRP(namers);

template<> template<>
void to::test<1>()
{
    auto_ptr<Namer> n(Namer::get(Namer::PLAIN));

    ensure_equals(n->name(WR_VAR(0, 1, 2)), "TODO_B01002_000");
    ensure_equals(n->name(WR_VAR(0, 1, 2)), "TODO_B01002_001");
    ensure_equals(n->name(WR_VAR(0, 1, 2), true), "TODO_B01002_002");
    ensure_equals(n->name(WR_VAR(0, 1, 2), true), "TODO_B01002_002");
}

}

// vim:set ts=4 sw=4:
