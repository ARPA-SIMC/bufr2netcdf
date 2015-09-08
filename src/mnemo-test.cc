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
        add_method("find", []() {
            const mnemo::Table* t = mnemo::Table::get(14);
            const char* val;

            val = t->find(WR_VAR(0, 0, 0));
            wassert(actual(val).isfalse());

            val = t->find(WR_VAR(0, 0, 1));
            wassert(actual(val) == "YTABAE");

            val = t->find(WR_VAR(0, 10, 1));
            wassert(actual(val) == "MHHA");

            val = t->find(WR_VAR(0, 40, 14));
            wassert(actual(val) == "MHFFST");

            // YSUPL is hardcoded for all C05000
            val = t->find(WR_VAR(2, 5, 0));
            wassert(actual(val) == "YSUPL");
        });
    }

} tests("mnemo");

}
