/*
 * wreport/tests - Unit test utilities
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <wreport/varinfo.h>
#include <wreport/var.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdio>

namespace wreport {
namespace tests {

#define ensure_contains(x, y) wreport::tests::impl_ensure_contains(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_contains(x, y) wreport::tests::impl_ensure_contains(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
static inline void impl_ensure_contains(const wibble::tests::Location& loc, const std::string& haystack, const std::string& needle)
{
	if( haystack.find(needle) == std::string::npos )
	{
		std::stringstream ss;
		ss << "'" << haystack << "' does not contain '" << needle << "'";
		throw tut::failure(loc.msg(ss.str()));
	}
}

#define ensure_not_contains(x, y) arki::tests::impl_ensure_not_contains(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_not_contains(x, y) arki::tests::impl_ensure_not_contains(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
static inline void impl_ensure_not_contains(const wibble::tests::Location& loc, const std::string& haystack, const std::string& needle)
{
	if( haystack.find(needle) != std::string::npos )
	{
		std::stringstream ss;
		ss << "'" << haystack << "' must not contain '" << needle << "'";
		throw tut::failure(loc.msg(ss.str()));
	}
}

#define ensure_varcode_equals(x, y) wreport::tests::_ensure_varcode_equals(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_varcode_equals(x, y) wreport::tests::_ensure_varcode_equals(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
static inline void _ensure_varcode_equals(const wibble::tests::Location& loc, Varcode actual, Varcode expected)
{
	if( expected != actual )
	{
		char buf[40];
		snprintf(buf, 40, "expected %01d%02d%03d actual %01d%02d%03d",
				WR_VAR_F(expected), WR_VAR_X(expected), WR_VAR_Y(expected),
				WR_VAR_F(actual), WR_VAR_X(actual), WR_VAR_Y(actual));
		throw tut::failure(loc.msg(buf));
	}
}

#define ensure_var_equals(x, y) wreport::tests::_ensure_var_equals(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_var_equals(x, y) wreport::tests::_ensure_var_equals(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
static inline void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, int val)
{
	inner_ensure_equals(var.enqi(), val);
}
static inline void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, double val)
{
	inner_ensure_equals(var.enqd(), val);
}
static inline void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, const std::string& val)
{
	inner_ensure_equals(std::string(var.enqc()), val);
}

#define ensure_var_undef(x) wreport::tests::_ensure_var_undef(wibble::tests::Location(__FILE__, __LINE__, #x " is undef"), (x))
#define inner_ensure_var_undef(x) wreport::tests::_ensure_var_undef(wibble::tests::Location(loc, __FILE__, __LINE__, #x " is undef"), (x))
static inline void _ensure_var_undef(const wibble::tests::Location& loc, const Var& var)
{
	inner_ensure_equals(var.value(), (const char*)0);
}

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
	LocalEnv(const std::string& key, const std::string& val)
		: key(key)
	{
		const char* v = getenv(key.c_str());
		oldVal = v == NULL ? "" : v;
		setenv(key.c_str(), val.c_str(), 1);
	}
	~LocalEnv()
	{
		setenv(key.c_str(), oldVal.c_str(), 1);
	}
};

} // namespace tests
} // namespace wreport

// vim:set ts=4 sw=4:
