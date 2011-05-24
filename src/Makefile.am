## Process this file with automake to produce Makefile.in

INCLUDES = $(WREPORT_CFLAGS)

#
# Convenience library with conversion code
#

noinst_HEADERS = convert.h utils.h

noinst_LTLIBRARIES = libb2nc.la
libb2nc_la_SOURCES = utils.cc convert.cc

#
# Conversion tool
#

bin_PROGRAMS = bufr2netcdf
bufr2netcdf_SOURCES = bufr2netcdf.cc
bufr2netcdf_LDADD = libb2nc.la $(NC_LIBS) $(WREPORT_LIBS)

#
# Unit testing
#

if DO_TESTS

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS += tests/tests.h

tut_test_SOURCES = \
	tests/tests.cc \
	tests/tut-main.cpp
tut_test_LDADD = \
	libb2nc.la \
	$(NC_LIBS) \
	$(WREPORT_LIBS) \
	$(WIBBLE_LIBS)

endif
