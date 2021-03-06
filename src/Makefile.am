## Process this file with automake to produce Makefile.in

SUBDIRS = nccmp .

AM_CPPFLAGS = $(WREPORT_CFLAGS) $(WIBBLE_CFLAGS) $(NETCDF_CFLAGS)

#
# Convenience library with conversion code
#

noinst_HEADERS = arrays.h compat.h convert.h mnemo.h namer.h ncoutfile.h options.h plan.h utils.h valarray.h

noinst_LTLIBRARIES = libb2nc.la
libb2nc_la_SOURCES = utils.cc mnemo.cc namer.cc valarray.cc plan.cc arrays.cc ncoutfile.cc convert.cc

#
# Conversion tool
#

bin_PROGRAMS = bufr2netcdf
bufr2netcdf_SOURCES = bufr2netcdf.cc
bufr2netcdf_LDADD = libb2nc.la $(NETCDF_LIBS) $(WREPORT_LIBS)

#
# Unit testing
#

#
# Unit tests
#

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/runtest

check-local:
	for test in $(check_PROGRAMS); do \
		$(TESTS_ENVIRONMENT) $$test ; \
	done

CXXFLAGS += -O0

noinst_HEADERS += tests/tests.h

tut_test_SOURCES = \
	mnemo-test.cc \
	namer-test.cc \
	ncoutfile-test.cc \
	valarray-test.cc \
	plan-test.cc \
	arrays-test.cc \
	convert-test.cc \
	tests/tests.cc \
	tests/tests-main.cc
tut_test_LDADD = \
	libb2nc.la \
	$(NETCDF_LIBS) \
	$(WREPORT_LIBS)
