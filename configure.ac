dnl Process this file with autoconf to produce a configure script.

AC_INIT(bufr2netcdf, 1.4, [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl To use subdirs
AC_PROG_MAKE_SET

AC_LANG([C++])

dnl Use c++11
AX_CXX_COMPILE_STDCXX_11

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX

AC_PROG_LIBTOOL

AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Needed by subdir-objects
AM_PROG_CC_C_O

dnl Check for missing functions in libc
AC_CHECK_FUNC(vasprintf, , [
	AC_DEFINE([USE_OWN_VASPRINTF], 1, [we need to use our own vasprintf])
])

dnl Check for missing functions in libc
has_getopt_long=no
AC_CHECK_FUNC(getopt_long, [
	AC_DEFINE([HAS_GETOPT_LONG], 1, [we can use long options])
	has_getopt_long=yes
])

dnl Check if std::vector has a std::vector::data method
AC_CACHE_CHECK(whether the compiler has std::vector::data (),
	ac_cv_cxx_have_vector_data,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <vector>
using namespace std;
],[vector<int> v; v.data(); return 0;],
 ac_cv_cxx_have_vector_data=yes, ac_cv_cxx_have_vector_data=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_vector_data" = yes; then
 AC_DEFINE(HAVE_VECTOR_DATA,,[define if the compiler has the method std::vector::data ()])
fi

dnl Check for wreport
PKG_CHECK_MODULES(WREPORT, [libwreport >= 3.0])

dnl Check for NetCDF
dnl pkg-config has conflicting cflags across distros, requiring includes in the
dnl form of netcdf/foo.h in Debian or just foo.h in Fedora
dnl PKG_CHECK_MODULES(NETCDF, [netcdf])
AC_CHECK_NETCDF()
AC_SUBST(NETCDF_CFLAGS)
AC_SUBST(NETCDF_LIBS)

dnl Target directory for DWD mnemonic table data
tabledir='${datadir}'"/$PACKAGE"
AC_SUBST(tabledir)

LIBS="$LIBS -lm"

dnl Enable all useful warnings
AX_CXXFLAGS_WARN_ALL

confdir='${sysconfdir}'"/$PACKAGE"
AC_SUBST(confdir)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/nccmp/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([
===================================================
$PACKAGE_NAME-$PACKAGE_VERSION configuration:
AS_HELP_STRING([getopt_long:], [$has_getopt_long])
===================================================])
