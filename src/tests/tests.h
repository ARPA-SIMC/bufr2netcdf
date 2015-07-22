/*
 * tests/tests - Unit test utilities
 *
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <wibble/tests.h>
#include <string>

namespace b2nc {
namespace tests {

/**
 * Return the pathname of a test file
 */
std::string datafile(const std::string& fname);

/**
 * Read the entire contents of a test file into a string
 *
 * The file name will be resolved through datafile
 */
std::string slurpfile(const std::string& name);


/// RAII-style override of an environment variable
class LocalEnv
{
    /// name of the environment variable that we override
    std::string key;
    /// stored original value of the variable
    std::string oldVal;

public:
    /**
     * @param key the environment variable to override
     * @param val the new value to assign to \a key
     */
    LocalEnv(const std::string& key, const std::string& val);
    ~LocalEnv();
};

} // namespace tests
} // namespace wreport

// vim:set ts=4 sw=4:
