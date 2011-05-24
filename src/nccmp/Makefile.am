## Process this file with automake to produce Makefile.in

if DO_TESTS

#
# nccmp comparison program
#

INCLUDES = $(NC_CFLAGS)

noinst_PROGRAMS = nccmp
nccmp_SOURCES = \
	getopt.c \
	nccmp.c \
	ncinfo.c \
	opt.c \
	strlist.c \
	xmalloc.c
nccmp_LDADD = $(NC_LIBS)

noinst_HEADERS = \
	common.h \
	getopt.h \
	nccmp.h \
	ncinfo.h \
	opt.h \
	strlist.h \
	xmalloc.h

EXTRA_DIST = README

endif
