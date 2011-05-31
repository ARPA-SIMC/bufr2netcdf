/*
 * mnemo - Mnemonic variable names resolution
 *
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

#ifndef B2NC_MNEMO_H
#define B2NC_MNEMO_H

#include <wreport/varinfo.h>
#include <vector>

namespace b2nc {
namespace mnemo {

/**
 * Name of environment variable used to override the mnemonic tables directory
 * pathname
 */
extern const char* ENV_TABLE_DIR;

/**
 * Default mnemonic tables directory location
 */
extern const char* DEFAULT_TABLE_DIR;

struct Record
{
    wreport::Varcode code;
    char name[10];

    Record();
    Record(wreport::Varcode code, const char* name);
    bool operator<(const Record& r) const;
};

class Table : protected std::vector<Record>
{
protected:
    int version;

    void load();

public:
    Table(int version);

    const char* find(wreport::Varcode code) const;

    static const Table* get(int version);
};

}
}

#endif

