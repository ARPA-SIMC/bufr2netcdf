/*
   Copyright (C) 2004-2006, 2009 Remik Ziemlinski <first d0t surname att n0aa d0t g0v>

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

#ifndef NCCMP_H
#define NCCMP_H 1
#include "opt.h"
#include <netcdf.h>
#include <math.h>

typedef struct VARSTRUCT
{
	char name[NC_MAX_NAME];
	int varid;
	off_t len; /* # elements per record (or overall if static). */
	size_t dimlens[NC_MAX_VAR_DIMS];
	nc_type type;
	int ndims;
	int dimids[NC_MAX_VAR_DIMS];
	char hasrec; /* has record dimension? */
	int natts;
} varstruct;

typedef struct DIMSTRUCT
{
	int dimid;
	size_t len;
	char name[NC_MAX_NAME];
} dimstruct;

int openfiles(nccmpopts* opts);
int nccmp(nccmpopts* opts); 
int nccmpmetadata(nccmpopts* opts); 
int nccmpdata(nccmpopts* opts); 
void getvarinfo(int ncid, varstruct* vars, int* nvars);
void type2string(nc_type type, char* str);
int excludevars(int ncid1, int ncid2, char** finallist,
                int nfinal, char** excludelist, int nexclude);


#endif /* !NCCMP_H */
