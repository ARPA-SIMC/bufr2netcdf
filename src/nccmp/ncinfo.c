/*
   Copyright (C) 2004, 2009 Remik Ziemlinski <first d0t surname att n0aa d0t g0v>

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

#include "ncinfo.h"

int ncnonrecvars(int ncid, char** list, int nlist)
{
	int status;
	int recid;
	int varid;
	int dimids[NC_MAX_VAR_DIMS];
	int ndims;
	int nvars;
	int natts;
	int i;
	nc_type type;
	char name[NC_MAX_NAME];
	
	/* Query the number of variables */
	status = nc_inq_nvars (ncid, &nvars);
	if (status != NC_NOERR) return EXIT_FATAL;      

	/* record dimension id */
	status = nc_inq_unlimdim(ncid, &recid);
	if (status == -1) 
	{
					/* no record dimension */
					recid = -1;
	}

	for(i=0; i < nvars; ++i)
	{
		/* Query the variable */
		varid = i;
		status = nc_inq_var(ncid, varid, name, &type,
													&ndims, dimids, &natts);
		if (status != NC_NOERR) return EXIT_FATAL;

		if (hasrecdim(dimids, ndims, recid) == 0)
		{
						addstringtolist(list, name, nlist);    
		}
	}            
	
	return EXIT_SUCCESS;
}
int ncrecvars(int ncid, char** list, int nlist)
{
	int status;
	int recid;
	int varid;
	int dimids[NC_MAX_VAR_DIMS];
	int ndims;
	int nvars;
	int natts;
	int i;
	nc_type type;
	char name[NC_MAX_NAME];

	/* Query the number of variables */
	status = nc_inq_nvars (ncid, &nvars);
	if (status != NC_NOERR) return EXIT_FATAL;      

	/* record dimension id */
	status = nc_inq_unlimdim(ncid, &recid);

	for(i=0; i < nvars; ++i)
	{
		/* Query the variable */
		varid = i;
		status = nc_inq_var(ncid, varid, name, &type,
													&ndims, dimids, &natts);
		if (status != NC_NOERR) return EXIT_FATAL;

		if (hasrecdim(dimids, ndims, recid))
						addstringtolist(list, name, nlist);    
	}            
	
	return EXIT_SUCCESS;
}
int ncallvars(int ncid, char** list, int nlist)
{
	int nrecvars;
	int nnonrecvars;
	char** nonrecvars = NULL;
	char** recvars = NULL;
	int status;
  
	status = EXIT_SUCCESS;
	newstringlist(&nonrecvars, &nnonrecvars, NC_MAX_VARS);
	newstringlist(&recvars, &nrecvars, NC_MAX_VARS);
	
	if(ncnonrecvars(ncid, nonrecvars, nnonrecvars))
		{
			status = EXIT_FATAL;
			goto recover;
		} 
	if(ncrecvars(ncid, recvars, nrecvars))
		{
			status = EXIT_FATAL;
			goto recover;
		} 
	
	/*printf("Nonrecvars:\n");
	printstrlist(nonrecvars, nnonrecvars, stdout);
	
	printf("Recvars:\n");
	printstrlist(recvars, nrecvars, stdout);
	
	printf("List (nlist=%d):\n", nlist);
	printstrlist(list, nlist, stdout);
	*/
	
	if(strlistu(   nonrecvars, recvars, list, nnonrecvars, nrecvars, nlist))
		{
			status = EXIT_FATAL;
			goto recover;
		} 

	/* printf("Nonrecvars 2:\n");
	printstrlist(nonrecvars, nnonrecvars, stderr);
	
	printf("Recvars 2:\n");
	printstrlist(recvars, nrecvars, stderr);
	*/
	
 recover:
	freestringlist(&recvars, nrecvars);                        
	freestringlist(&nonrecvars, nnonrecvars);                        
	
	/* printf("ncallvars, nlist = %d\n", nlist); */

	return status;
}
int ncrecinfo(int ncid, int* recid, char* name, size_t* size)
{
	int status;
	
	/* get ID of unlimited dimension */
	status = nc_inq_unlimdim(ncid, recid); 
	if (status != NC_NOERR) return EXIT_FATAL;

	if (*recid < 0) 
	{
		/* no record */
		strcpy(name,"");
		*size = 0;
		return EXIT_SUCCESS; 
	}
			
	/* Query the dimension */
	status = nc_inq_dim (ncid, *recid, name, size);
	if (status != NC_NOERR) return EXIT_FATAL;

	return EXIT_SUCCESS;
}
int hasrecdim(int* dimids, int ndims, int recid)
{
	int i;
	
	if ( (ndims < 1) || (recid < 0) )
					return 0;
	
	for(i = 0; i < ndims; ++i)
	{
		if (dimids[i] == recid)
		{
						return 1;
		}
	}
	
	return 0;
}
int gettypelength(nc_type type)
{
        switch(type)
        {
                case NC_BYTE:
                case NC_CHAR:
                return 1;
                break;
                case NC_SHORT:
                return 2;
                break;
                case NC_INT:
                case NC_FLOAT:
                return 4;
                break;
                case NC_DOUBLE:
                return 8;
                break;
        }
        return 0;
}
