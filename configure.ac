dnl Process this file with autoconf to produce a configure script.

AC_INIT(bufr2netcdf, 0.1, [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl To use subdirs
AC_PROG_MAKE_SET

AC_LANG([C++])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX

AC_PROG_LIBTOOL

AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Needed by subdir-objects
AM_PROG_CC_C_O

dnl Check for wibble
PKG_CHECK_EXISTS([libwibble], [have_wibble=yes], [have_wibble=no])
if test x$have_wibble = xyes
then
    PKG_CHECK_MODULES(WIBBLE,libwibble)
    enable_tests=yes
else
    AC_MSG_WARN([libwibble not found, tests will be disabled])
    enable_tests=no
fi

dnl Check for wreport
PKG_CHECK_MODULES(WREPORT, [libwreport >= 1.7])

dnl Check for NetCDF
PKG_CHECK_MODULES(NC, [netcdf])

LIBS="$LIBS -lm"

confdir='${sysconfdir}'"/$PACKAGE"
AC_SUBST(confdir)

CFLAGS="-Wall -Werror $CFLAGS"
CXXFLAGS="-Wall -Werror $CFLAGS"

AM_CONDITIONAL([DO_TESTS], [test x"$enable_tests" = x"yes"])

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([
===================================================
$PACKAGE_NAME-$PACKAGE_VERSION configuration:
AS_HELP_STRING([tests:], [$enable_tests])
===================================================])
