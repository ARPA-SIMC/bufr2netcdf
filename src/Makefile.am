## Process this file with automake to produce Makefile.in

INCLUDES = $(WREPORT_CFLAGS)

bin_PROGRAMS = bufr2netcdf

bufr2netcdf_SOURCES = bufr2netcdf.cc
bufr2netcdf_LDADD = $(WREPORT_LIBS)

#
# Unit testing
#

if DO_TESTS

check_PROGRAMS = tut_test
TESTS_ENVIRONMENT = $(top_srcdir)/runtest
TESTS = $(check_PROGRAMS)

CXXFLAGS += -O0

noinst_HEADERS = tests/tests.h

tut_test_SOURCES = \
	tests/tut-main.cpp
tut_test_LDADD = \
	$(WREPORT_LIBS) \
	$(WIBBLE_LIBS)

endif
