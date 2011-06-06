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

#include "valarray.h"
#include <tests/tests.h>
#include <wreport/error.h>
#include <wreport/bulletin.h>
#include <cstdio>

using namespace b2nc;
using namespace wreport;
using namespace wibble;
using namespace std;

namespace tut {

struct valarray_shar
{
    const Vartable* table;

    valarray_shar()
    {
        table = Vartable::get("B0000000000000014000");
        ensure(table);
    }

    ~valarray_shar()
    {
    }
};
TESTGRP(valarray);

template<> template<>
void to::test<1>()
{
    Var var(table->query(WR_VAR(0, 1, 1)));
    auto_ptr<ValArray> arr(ValArray::make_singlevalarray(Namer::DT_DATA, var.info()));
    // TODO
}

template<> template<>
void to::test<2>()
{
    LoopInfo loopinfo("foo", 0);
    Var var(table->query(WR_VAR(0, 1, 15)));
    auto_ptr<ValArray> arr(ValArray::make_multivalarray(Namer::DT_DATA, var.info(), loopinfo));
    // TODO
}

}

// vim:set ts=4 sw=4:

