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

#ifndef NCINFO_H
#define NCINFO_H 1

#include <netcdf.h>
#include "common.h"

/* list must be preallocated */
int ncnonrecvars(int ncid, char** list, int nitems);
/* list must be preallocated */
int ncrecvars(int ncid, char** list, int nitems);
/* list must be preallocated */
int ncallvars(int ncid, char** list, int nlist);
int ncrecinfo(int ncid, int* recid, char* name, size_t* size);
int gettypelength(nc_type type);
#endif /* !NCINFO_H */
