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

#include "mnemo.h"
#include <tests/tests.h>
#include <wibble/string.h>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct mnemo_shar
{
    mnemo_shar()
    {
    }

    ~mnemo_shar()
    {
    }
};
TESTGRP(mnemo);

template<> template<>
void to::test<1>()
{
    const mnemo::Table* t = mnemo::Table::get(14);
    const char* val;

    val = t->find(WR_VAR(0, 0, 0));
    ensure(!val);

    val = t->find(WR_VAR(0, 0, 1));
    ensure(val);
    ensure_equals(string(val), "YTABAE");

    val = t->find(WR_VAR(0, 10, 1));
    ensure(val);
    ensure_equals(string(val), "MHHA");

    val = t->find(WR_VAR(0, 40, 14));
    ensure(val);
    ensure_equals(string(val), "MHFFST");
}

}

// vim:set ts=4 sw=4:
