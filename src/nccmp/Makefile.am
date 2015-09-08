## Process this file with automake to produce Makefile.in

#
# nccmp comparison program
#

AM_CPPFLAGS = $(NETCDF_CFLAGS)

noinst_PROGRAMS = nccmp
nccmp_SOURCES = \
	getopt.c \
	nccmp.c \
	ncinfo.c \
	opt.c \
	strlist.c \
	xmalloc.c
nccmp_LDADD = $(NETCDF_LIBS)

noinst_HEADERS = \
	common.h \
	getopt.h \
	nccmp.h \
	ncinfo.h \
	opt.h \
	strlist.h \
	xmalloc.h

EXTRA_DIST = README
