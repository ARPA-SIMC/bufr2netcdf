/*
   Copyright (C) 2004,2009 Remik Ziemlinski <first d0t surname att n0aa d0t g0v>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

/*
        Option & argument parsing from command-line.        
*/
#ifndef OPT_H
#define OPT_H 1

#include "common.h"
#include "getopt.h"
#include "strlist.h"
#include "netcdf.h"

/* macro is defined at compile time from within Makefile, not config.h */
#define USAGE "\
Compare two NetCDF files.\n\
nccmp is silent when files are equivalent, otherwise it will echo to STDERR whether metadata or a specific variable differs.  By default, comparing stops after the first difference.\n\
\n\
Exit code 0 is returned for identical files, 1 for different files, and 2 for a fatal error.\n\
\n\
Usage: nccmp [OPTION...] file1 file2\n\
\n\
  -A, --Attribute=att1[,...] Ignore attribute(s) att1[,...]\n\
                             for all variables.\n\
  -b, --verbose              Print messages to show progress.\n\
  -d, --data                 Compare data (variable values).\n\
  -D, --debug                Prints verbose debug messages.\n\
  -f, --force                Forcefully compare, do not stop after first difference.\n\
  -F, --fortran              Print position indices using Fortran style (1-based reverse order).\n\
  -g, --global               Compare global attributes.\n\
                             (-m required)\n\
  -G, --globalex att1[,...]  Exclude global attributes att1[,...].\n\
                             (-mg required)\n\
  -h, --history              Compare global history attribute.\n\
                             (-mg required)\n\
  -H, --help                 Give this usage message.\n\
      --usage                \n\
  -m, --metadata             Compare metadata, excluding global attributes.\n\
  -p, --precision='%%.17g'    Precision of difference printing (default is '%%g').\n\
                             Use '%%x' to print bytes.\n\
  -t, --tolerance=TOL        Compare if absolute difference > TOL.\n\
  -T, --Tolerance=TOL        Compare if relative percent difference > TOL.\n\
  -v, --variable=var1[,...]  Compare variable(s) var1[,...] only.\n\
  -V, --version              Print program version.\n\
  -x, --exclude=var1[,...]   Exclude variable(s) var1[,...].\n\
  -w, --warn=tag[,...]       Selectively make certain differences exceptions,\n\
                             so only informational messages are printed for them.\n\
                             Supported tags and their meaning:\n\
                             all: All differences.\n\
                             format: File formats.\n\
\n\
Report bugs to Remik Ziemlinski @ noaa gov.\n\
"

/* used in allocating a string list */
#define NCCMP_MAX_STRINGS 256

/* Warning tags supported, which are used as array indices. */
#define NCCMP_W_TAGS "all,format"
#define NCCMP_W_NUMTAGS 2
#define NCCMP_W_ALL 0
#define NCCMP_W_FORMAT 1

typedef struct NCCMPOPTS
{
	char**  excludeattlist; /* variable attributes to exclude from comparisons */
	int     nexcludeatt;    /* number of strings in excludeattlist */
	int     data;           /* compare variable values? */
	int     debug;          /* print additional debug messages */
	int     force;          /* continue checking after 1st difference */
	int     fortran;        /* fortran style index printing */
	int     global;         /* compare global attributes */
  int     nglobalexclude; /* number of global atts to exclude */
	char**  globalexclude;  /* list of global atts to exclude */
	int     history;        /* compare global history attribute */
	int     metadata;       /* compare metadata, excluding global attributes */
	int     quiet;          /* deprecated. prints differences only, no script making for viewing */
	char*   precision;      /* print precision of values. */
	int     verbose;        /* print messages to show progress */
	int     help;           /* give usage insructions */
	int     version;        /* give program version */
	int     exclude;        /* exclude list given */
	double  tolerance;      /* allowable difference tolerance */
	char    abstolerance;   /* flag, 0=percentage (0-100+), 1=absolute tolerance */
	int     variable;       /* variable list given */
	int     nexclude;       /* how many to exclude */
	int     nvariable;      /* how many variables */
	char**  excludelist;    /* variables to exclude from comparison */
	char**  variablelist;   /* specific variables to compare */
	char*   file1;          /* two files to compare */
	char*   file2;          /*                      */
  char**  cmpvarlist;     /* list of var names to ultimately compare. */
                          /* Not user input. Derived by processing inputs. */
  int     ncmpvarlist;    /* length of above array. */
  char    warn[NCCMP_W_NUMTAGS];   /* booleans if a warning is on/off */
} nccmpopts;

int getnccmpopts(int argc, char** argv, nccmpopts* popts);
void printusage();
void printversion();
/* frees pointers in struct */
void freenccmpopts(nccmpopts* popts);
void initnccmpopts(nccmpopts* popts);

#endif /* !OPT_H */
