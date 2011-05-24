/*
   Copyright (C) 2004-2007,2009,2010 Remik Ziemlinski @ noaa gov

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

#include "nccmp.h"
#include "ncinfo.h"

int ncid1, ncid2;
varstruct vars1[(int)NC_MAX_VARS], vars2[(int)NC_MAX_VARS];
dimstruct dims1[(int)NC_MAX_DIMS], dims2[(int)NC_MAX_DIMS];
size_t nrec1, nrec2;
int nvars1, nvars2, ndims1, ndims2, recid1, recid2;

#define NCFORMATSTR(f) 																					\
		(f == NC_FORMAT_CLASSIC ? "NC_FORMAT_CLASSIC" :								\
			(f == NC_FORMAT_64BIT ? "NC_FORMAT_64BIT" :									\
			(f == NC_FORMAT_NETCDF4 ? "NC_FORMAT_NETCDF4" :							\
		 "NC_FORMAT_NETCDF4_CLASSIC")))																	\

/* *********************************************************** */
/* Returns formatted string of dimension indices.
	Intended to print out locations of value differences. 
	'out' should be pre-allocated large enough for output. */
void getidxstr(varstruct* var, size_t* start, int curidx, char* out)
{
	int i;
	char tmp[8];
	memset(out,'\0',32);
	for(i=0; i < var->ndims-1; ++i) {
		sprintf(tmp, "%d ", (int)start[i]);
		strcat(out, tmp);
	}
	sprintf(tmp, "%d", curidx);
	strcat(out, tmp);
}
/* *********************************************************** */
/* Same as above but with fortran style indices, which means they're
  1-based (i.e. first index is 1, not 0) and fast-varying dimension is
  printed first (reverse order compared with C-style indices) */
void getidxstr_fortran(varstruct* var, size_t* start, int curidx, char* out)
{
	int i;
	char tmp[8];
	memset(out,'\0',32);
	sprintf(tmp, "%d", curidx + 1);
	strcat(out, tmp);
	
	for(i = var->ndims-2; i >= 0; --i) {
		sprintf(tmp, " %d", (int)start[i]+1);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
/* Creates a string representation of a value into pre-allocated char array. */
void doubleToString(double v, char* out, char* formatprec) { \
	sprintf(out, formatprec, v);
}
/* *********************************************************** */
void floatToString(float v, char* out, char* formatprec) { \
	sprintf(out, formatprec, (double)v);
}
/* *********************************************************** */
void intToString(int v, char* out, char* ignore) { \
	sprintf(out, "%d", v);
}
/* *********************************************************** */
void shortToString(short v, char* out, char* ignore) { \
	sprintf(out, "%d", (int)v);
}
/* *********************************************************** */
void ucharToString(unsigned char v, char* out, char* ignore) { \
	sprintf(out, "%c", v);
}
/* *********************************************************** */
void textToString(char v, char* out, char* ignore) { \
	sprintf(out, "%c", v);
}
/* *********************************************************** */
/* Creates a hex representation of a value into pre-allocated char array. */
void doubleToHex(double v, char* out) { \
	unsigned char *p = (unsigned char*)&v; 
	int i; 
	char tmp[3];
	
	strcpy(out, "0x");
	
	for(i = 0; i < sizeof(double); ++i) {
		sprintf(tmp, "%02X", p[i]);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
void floatToHex(float v, char* out) { \
	unsigned char *p = (unsigned char*)&v; 
	int i; 
	char tmp[3];
	
	strcpy(out, "0x");
	
	for(i = 0; i < sizeof(float); ++i) {
		sprintf(tmp, "%02X", p[i]);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
void intToHex(int v, char* out) { \
	unsigned char *p = (unsigned char*)&v; 
	int i; 
	char tmp[3];
	
	strcpy(out, "0x");
	
	for(i = 0; i < sizeof(int); ++i) {
		sprintf(tmp, "%02X", p[i]);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
void shortToHex(short v, char* out) { \
	unsigned char *p = (unsigned char*)&v; 
	int i; 
	char tmp[3];
	
	strcpy(out, "0x");
	
	for(i = 0; i < sizeof(short); ++i) {
		sprintf(tmp, "%02X", p[i]);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
void ucharToHex(unsigned char v, char* out) { \
	unsigned char *p = (unsigned char*)&v; 
	int i; 
	char tmp[3];
	
	strcpy(out, "0x");
	
	for(i = 0; i < sizeof(unsigned char); ++i) {
		sprintf(tmp, "%02X", p[i]);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
void textToHex(char v, char* out) { \
	unsigned char *p = (unsigned char*)&v; 
	int i; 
	char tmp[3];
	
	strcpy(out, "0x");
	
	for(i = 0; i < sizeof(char); ++i) {
		sprintf(tmp, "%02X", p[i]);
		strcat(out, tmp);
	}
}
/* *********************************************************** */
int 
excludevars(int ncid1, int ncid2, char** finallist,
                int nfinal, char** excludelist, int nexclude)
{
	int nvars;
	char** vars = NULL;
	int status = EXIT_SUCCESS;
	
	vars = NULL;
	
	if (    (nexclude == 0)
		)        
					return EXIT_SUCCESS;

	/* printf("%d: creating temporary var list array.\n", __LINE__); */
	/* get simple difference */
	if(newstringlist(&vars, &nvars, NC_MAX_VARS))
		status = EXIT_FATAL;
	
	/* printf("%d: getting all variables names from both input files.\n", __LINE__); */
	if(allvarnames(vars, nvars))
		status = EXIT_FATAL; 
	
	/*printf("vars=");
	 printstrlist(vars, nvars, stdout);
	*/
		
	if(strlistsd(  vars, excludelist, finallist,
								nvars, nexclude,   nfinal))
		status = EXIT_FATAL; 
	
	/*printf("excludelist=");
	 printstrlist(excludelist, nexclude, stdout);
	*/
	
	/*printf("finallist=");
	 printstrlist(finallist, nfinal, stdout);
	*/
	
	freestringlist(&vars, nvars);         

	return status;                               
}
/* *********************************************************** */
void handle_error(int status) {
     if (status != NC_NOERR) {
        fprintf(stderr, "%s\n", nc_strerror(status));
        exit(-1);
        }
     }

/* *********************************************************** */
/*
	Mimics incrementing odometer. 
	Returns 0 if rolled-over. 
	
	@param odo: the counters that are updated.
	@param limits: the maximum values allowed per counter.
	@param first: index of first counter to update.
	@param last: index of last counter to update.
	*/
int odometer(size_t* odo, size_t* limits, int first, int last)
{
	int i = last;
	while (i >= first) {
		odo[i] += 1;
		if (odo[i] > limits[i]) 
			odo[i] = 0;
		else
			break;
		
		--i;
	}

#ifdef __DEBUG__
	printf("DEBUG : %d : odo = ", __LINE__);
	for(i=first; i <= last; ++i) {
		printf("%d ", odo[i]);
	}
	printf("\n");
#endif

	/* Test for roll over. */
	for(i=first; i <= last; ++i) {
		if (odo[i] != 0)
			return 1;
	}
		
	/* If we get here then rolled over. */
	return 0;
}
/* *********************************************************** */
/* Pretty prints attribute values into a string. 
	Assumes 'str' is pre-allocated large enough to hold output string.
*/
void prettyprintatt(int ncid, char* varname, int varid, char* name, char* str) 
{
	int status, i;
	nc_type type;
	size_t len;
	char* pc;
	unsigned char* puc;
	short* ps;
	int* pi;
	float* pf;
	double* pd;
	char tmpstr[32];
        
	status = nc_inq_att(ncid, varid, name, &type, &len);
	if (status != NC_NOERR) {
		if (varid == NC_GLOBAL)
			fprintf(stderr, "ERROR : QUERYING GLOBAL ATTRIBUTE \"%s\"\n", name);
		else
			fprintf(stderr, "ERROR : QUERYING ATTRIBUTE \"%s\" FOR VARIABLE \"%s\"\n", name, varname);
		return;          
	}
	
	str[0] = '\0';
	if (len < 1) {
		return;
	}
	
	switch(type) 
	{
	case NC_BYTE:
		puc = XMALLOC(unsigned char,len);
		
		status = nc_get_att_uchar(ncid, varid, name, puc);
		if (status != NC_NOERR)
		{
			XFREE(puc); return;
		}
		
		for(i=0; i < (int)len; ++i) {
			ucharToString(puc[i], str+2*i, NULL);
			str[2*i+1] = ',';
		}
			
		XFREE(puc); 
		str[2*(int)len-1] = '\0';
		break;
	
	case NC_CHAR:
		pc = XMALLOC(char,len);
		status = nc_get_att_text(ncid, varid, name, pc);
		if (status != NC_NOERR)
		{
			XFREE(pc); return;
		}

		for(i=0; i < (int)len; ++i)
			textToString(pc[i], str+i, NULL);

		XFREE(pc); 
		str[(int)len] = '\0';
		break;
		
	case NC_SHORT:
		ps = XMALLOC(short,len);
		status = nc_get_att_short(ncid, varid, name, ps);
		if (status != NC_NOERR)
		{
			XFREE(ps); return;
		}

		for(i=0; i < (int)len; ++i) {
			sprintf(tmpstr, "%d,", ps[i]);
			strcat(str, tmpstr);
		}
		
		XFREE(ps); 
		str[strlen(str)-1] = '\0'; // Remove last comma.
		break;        

	case NC_INT:
		pi = XMALLOC(int,len);
		status = nc_get_att_int(ncid, varid, name, pi);
		if (status != NC_NOERR)
		{
			XFREE(pi); return;
		}

		for(i=0; i < (int)len; ++i) {
			sprintf(tmpstr, "%d,", pi[i]);
			strcat(str, tmpstr);
		}
		
		XFREE(pi); 
		str[strlen(str)-1] = '\0'; // Remove last comma.
		break;        

	case NC_FLOAT:
		pf = XMALLOC(float,len);
		status = nc_get_att_float(ncid, varid, name, pf);
		if (status != NC_NOERR)
		{
			XFREE(pf); return;
		}

		for(i=0; i < (int)len; ++i) {
			sprintf(tmpstr, "%.9g,", pf[i]);
			strcat(str, tmpstr);
		}
		
		XFREE(pf); 
		str[strlen(str)-1] = '\0'; // Remove last comma.
		break;        

	case NC_DOUBLE:
		pd = XMALLOC(double,len);
		status = nc_get_att_double(ncid, varid, name, pd);
		if (status != NC_NOERR)
		{
			XFREE(pd); return;
		}

		for(i=0; i < (int)len; ++i) {
			sprintf(tmpstr, "%.17g,", pd[i]);
			strcat(str, tmpstr);
		}
		
		XFREE(pd); 
		str[strlen(str)-1] = '\0'; // Remove last comma.
		break;        
	}
}
/* *********************************************************** */
int cmpatt(int varid1, int varid2, 
					 char* name, char* varname, nccmpopts* opts)
{
	int ncstatus, status, attid1, attid2;
	nc_type type1, type2;
	size_t lenp1, lenp2;
	char typestr1[256];
	char typestr2[256];

	status = EXIT_SUCCESS;
	ncstatus = nc_inq_att(ncid1, varid1, name, &type1, &lenp1);
	if (ncstatus != NC_NOERR) {
		fprintf(stderr, "DIFFER : VARIABLE \"%s\" IS MISSING ATTRIBUTE WITH NAME \"%s\" IN FILE \"%s\"\n", varname, name, opts->file1);
		
		if (!opts->warn[NCCMP_W_ALL]) 
			status = EXIT_DIFFER;

		return status;          
	}
	
	ncstatus = nc_inq_att(ncid2, varid2, name, &type2, &lenp2);
	if (ncstatus != NC_NOERR) {
		fprintf(stderr, "DIFFER : VARIABLE \"%s\" IS MISSING ATTRIBUTE WITH NAME \"%s\" IN FILE \"%s\"\n", varname, name, opts->file2);
		
		if (!opts->warn[NCCMP_W_ALL]) 
			status = EXIT_DIFFER;

		return status;          
	}

	if (type1 != type2)	{
			type2string(type1, typestr1);
			type2string(type2, typestr2);                                                
			fprintf(stderr, "DIFFER : TYPES : ATTRIBUTE : %s : VARIABLE : %s : %s <> %s\n", 
							name, varname, typestr1, typestr2);
							
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;

			return status;
		}

	if (lenp1 != lenp2) {
			/*fprintf(stderr, "DIFFER : LENGTHS : ATTRIBUTE : %s : VARIABLE : %s : %lu <> %lu\n", 
							name, varname, (unsigned long)lenp1, (unsigned long)lenp2);
			*/
			prettyprintatt(ncid1, varname, varid1, name, typestr1);
			prettyprintatt(ncid2, varname, varid2, name, typestr2);
			fprintf(stderr, "DIFFER : VARIABLE : %s : ATTRIBUTE : %s : VALUES : %s <> %s\n", varname, name, typestr1, typestr2);
					
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;

			return status;
		}

	if (cmpattval(ncid1, ncid2, varid1, varid2, name, lenp1, type1) != NC_NOERR) {
			prettyprintatt(ncid1, varname, varid1, name, typestr1);
			prettyprintatt(ncid2, varname, varid2, name, typestr2);
			fprintf(stderr, "DIFFER : VARIABLE : %s : ATTRIBUTE : %s : VALUES : %s <> %s\n", varname, name, typestr1, typestr2);

			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;

			return status;
		}
	
	return EXIT_SUCCESS;
}


/* *********************************************************** */
/* Assumes that both attributes have same length.
 */
int cmpattval(int nc1, int nc2, int varid1, int varid2, char* name, int len, nc_type type)
{
        char* c1;
        unsigned char* uc1;
        short* s1;
        int* i1;
        float* f1;
        double* d1;
        char* c2;
        unsigned char* uc2;
        short* s2;
        int* i2;
        float* f2;
        double* d2;
        int status;
        int i;
       
        if (name == NULL) return NC_EINVAL;
       
        switch(type)
        {
        case NC_BYTE:
                uc1 = XMALLOC(unsigned char,len);
                uc2 = XMALLOC(unsigned char,len);                
                status = nc_get_att_uchar(nc1, varid1, name, uc1);
                if (status != NC_NOERR)
                {
                        XFREE(uc1); XFREE(uc2); return NC_EINVAL;
                }
                status = nc_get_att_uchar(nc2, varid2, name, uc2);
                if (status != NC_NOERR)
                {
                        XFREE(uc1); XFREE(uc2); return NC_EINVAL;
                }
                if (strncmp((const char*)uc1,(const char*)uc2,len) != 0)
                {
                        XFREE(uc1); XFREE(uc2); return EXIT_DIFFER;
                }
               XFREE(uc1); 
               XFREE(uc2); 
        break;
        case NC_CHAR:
                c1 = XMALLOC(char,len);
                c2 = XMALLOC(char,len);                
                status = nc_get_att_text(nc1, varid1, name, c1);
                if (status != NC_NOERR)
                {
                        XFREE(c1); XFREE(c2); return NC_EINVAL;
                }
                status = nc_get_att_text(nc2, varid2, name, c2);
                if (status != NC_NOERR)
                {
                        XFREE(c1); XFREE(c2); return NC_EINVAL;
                }
                if (strncmp(c1,c2,len) != 0)
                {
                        XFREE(c1); XFREE(c2); return EXIT_DIFFER;
                }
               XFREE(c1); 
               XFREE(c2); 
        break;
        case NC_SHORT:
                s1 = XMALLOC(short,len);
                s2 = XMALLOC(short,len);                
                status = nc_get_att_short(nc1, varid1, name, s1);
                if (status != NC_NOERR)
                {
                        XFREE(s1); XFREE(s2); return NC_EINVAL;
                }
                status = nc_get_att_short(nc2, varid2, name, s2);
                if (status != NC_NOERR)
                {
                        XFREE(s1); XFREE(s2); return NC_EINVAL;
                }
                for(i = 0; i < len; ++i)
                {
                        if (s1[i] != s2[i])
                        {
                                XFREE(s1); XFREE(s2); return EXIT_DIFFER;
                        }
                }
               XFREE(s1); 
               XFREE(s2); 
        break;        
        case NC_INT:
                i1 = XMALLOC(int,len);
                i2 = XMALLOC(int,len);                
                status = nc_get_att_int(nc1, varid1, name, i1);
                if (status != NC_NOERR)
                {
                        XFREE(i1); XFREE(i2); return NC_EINVAL;
                }
                status = nc_get_att_int(nc2, varid2, name, i2);
                if (status != NC_NOERR)
                {
                        XFREE(i1); XFREE(i2); return NC_EINVAL;
                }
                for(i = 0; i < len; ++i)
                {
                        if (i1[i] != i2[i])
                        {
                                XFREE(i1); XFREE(i2); return EXIT_DIFFER;
                        }
                }
               XFREE(i1); 
               XFREE(i2); 
        break;        
        case NC_FLOAT:
                f1 = XMALLOC(float,len);
                f2 = XMALLOC(float,len);                
                status = nc_get_att_float(nc1, varid1, name, f1);
                if (status != NC_NOERR)
                {
                        XFREE(f1); XFREE(f2); return NC_EINVAL;
                }
                status = nc_get_att_float(nc2, varid2, name, f2);
                if (status != NC_NOERR)
                {
                        XFREE(f1); XFREE(f2); return NC_EINVAL;
                }
                for(i = 0; i < len; ++i)
                {
                        if (f1[i] != f2[i])
                        {
                                XFREE(f1); XFREE(f2); return EXIT_DIFFER;
                        }
                }
               XFREE(f1); 
               XFREE(f2); 
        break;        
        case NC_DOUBLE:
                d1 = XMALLOC(double,len);
                d2 = XMALLOC(double,len);                
                status = nc_get_att_double(nc1, varid1, name, d1);
                if (status != NC_NOERR)
                {
                        XFREE(d1); XFREE(d2); return NC_EINVAL;
                }
                status = nc_get_att_double(nc2, varid2, name, d2);
                if (status != NC_NOERR)
                {
                        XFREE(d1); XFREE(d2); return NC_EINVAL;
                }
                for(i = 0; i < len; ++i)
                {
                        if (d1[i] != d2[i])
                        {
                                XFREE(d1); XFREE(d2); return EXIT_DIFFER;
                        }
                }
               XFREE(d1); 
               XFREE(d2); 
        break;        
        }
        
        return EXIT_SUCCESS;
}
/* *********************************************************** */
void type2string(nc_type type, char* str)
{
        switch(type)
        {
        case NC_BYTE:
	        strcpy(str, "BYTE");
  	      break;
        case NC_CHAR:
    	    strcpy(str, "CHAR");
      	  break;
        case NC_SHORT:      
	        strcpy(str, "SHORT");
  	      break;        
        case NC_INT:  
	        strcpy(str, "INT");  
  	      break;        
        case NC_FLOAT:     
    	    strcpy(str, "FLOAT");
      	  break;        
        case NC_DOUBLE:
	        strcpy(str, "DOUBLE");
  	      break;
        default:
        	strcpy(str, "");
        	break;
        }                 
} 
/* *********************************************************** */
int openfiles(nccmpopts* opts)
{
	int status;

	status = nc_open(opts->file1, NC_NOWRITE, &ncid1);
	handle_error(status);

	status = nc_open(opts->file2, NC_NOWRITE, &ncid2);
	handle_error(status);

	return 0;
}
/* *********************************************************** */
/* Compares record names and lengths. */
int
nccmprecinfo(nccmpopts* opts)
{
	char name1[256], name2[256];
	int status;

	status = EXIT_SUCCESS;
	
	if (opts->verbose)
		printf("INFO: Comparing record information.\n");

	status = nc_inq_unlimdim(ncid1, &recid1);
	handle_error(status);

	if (recid1 != -1) {
		status = nc_inq_dim(ncid1, recid1, name1, &nrec1);
		handle_error(status);
	} else {
		nrec1 = 0;
	}

	status = nc_inq_unlimdim(ncid2, &recid2);
	handle_error(status);

	if (recid2 != -1) {
		status = nc_inq_dim(ncid2, recid2, name2, &nrec2);
		handle_error(status);
	} else {
		nrec2 = 0;
	}

	if (instringlist(opts->excludelist, name1, opts->nexclude) ||
			instringlist(opts->excludelist, name2, opts->nexclude) ||
			!instringlist(opts->variablelist, name1, opts->nvariable) ||
			!instringlist(opts->variablelist, name2, opts->nvariable))
		return EXIT_SUCCESS;

	if (strcmp(name1, name2)) {
		fprintf(stderr, "DIFFER : NAMES OF RECORDS : %s <> %s\n", name1, name2);
		if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;

		if(!opts->force) return status;
	}

	if (nrec1 != nrec2) {
		fprintf(stderr, "DIFFER : LENGTHS OF RECORDS : %s (%d) <> %s (%d)\n", name1, (int)nrec1, name2, (int)nrec2);
		if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;

		if(!opts->force) return status;
	}

	return status;
}
/* *********************************************************** */
/* Get dim info for file. */
void getdiminfo(int ncid, dimstruct* dims, int* ndims)
{
	int status, i;

	status = nc_inq_ndims(ncid, ndims);
	handle_error(status);

	for(i=0; i < *ndims; ++i) {
		dims[i].dimid = i;
		status = nc_inq_dim(ncid, i, dims[i].name, &dims[i].len);
		handle_error(status);
	}
}

/* *********************************************************** */
/* Read all the file's metadata for variables. */
void getvarinfo(int ncid, varstruct* vars, int* nvars)
{
	int status, i, j, recid;
	char name[NC_MAX_NAME];

	status = nc_inq_nvars(ncid, nvars);
	handle_error(status);

	status = nc_inq_unlimdim(ncid, &recid);
	handle_error(status);

	for(i=0; i < *nvars; ++i) {
		vars[i].varid = i;

		status = nc_inq_var(ncid, i, vars[i].name, &vars[i].type,
												&vars[i].ndims, vars[i].dimids, &vars[i].natts);
		handle_error(status);

		vars[i].len = 1;
		for(j=0; j < vars[i].ndims; ++j) {
			status = nc_inq_dimlen(ncid, vars[i].dimids[j], &vars[i].dimlens[j]);
			handle_error(status);
#ifdef __DEBUG__
			printf("DEBUG : %d : %s dimid %d, len %d\n", __LINE__, vars[i].name, j, vars[i].dimlens[j]);
#endif
			vars[i].len *= vars[i].dimlens[j];
		}

		vars[i].hasrec = (vars[i].dimids[0] == recid);
	}
}
/* *********************************************************** */
/* Returns index to varstruct in list, otherwise -1. */
int isinvarstructlist(char* name, varstruct* vars, int nvars) 
{
	int i;

	for(i=0; i < nvars; ++i) {
		if (strcmp(name, vars[i].name) == 0) 
			return i;
	}

	return -1;
}
/* *********************************************************** */
/* Get vars to use to do cmp based on input and exclusion lists. */
int makecmpvarlist(nccmpopts* opts)
{
	int status, i;

	newstringlist(&opts->cmpvarlist, &opts->ncmpvarlist, (int)NC_MAX_VARS);
	if(opts->variable) {
		if (opts->verbose)
			printf("INFO: Using variables provided in list.\n");

		status = strlistu(opts->variablelist, opts->cmpvarlist, opts->cmpvarlist, 
											opts->nvariable, opts->ncmpvarlist, opts->ncmpvarlist);
	} else if (opts->exclude) {
		if (opts->verbose)
			printf("INFO: Excluding variables in provided list.\n");

		status = excludevars(ncid1, ncid2, opts->cmpvarlist, opts->ncmpvarlist, opts->excludelist, opts->nexclude);
	} else {
		if (opts->verbose)
			printf("INFO: Using all variables.\n");

		status = allvarnames(opts->cmpvarlist, opts->ncmpvarlist);
	}

	opts->ncmpvarlist = getnumstrlist(opts->cmpvarlist, (int)NC_MAX_VARS);
	if (opts->verbose) {
		printf("INFO: Variables to compare (%d):\n", opts->ncmpvarlist);
		for(i=0; i < opts->ncmpvarlist-1; ++i)
			printf("%s, ", opts->cmpvarlist[i]);
		
		if (opts->ncmpvarlist)
			printf("%s\n", opts->cmpvarlist[i]);
	}
	
	return status;
}
/* *********************************************************** */
/* Gets list of all variable names in both input files. */
int 
allvarnames(char** list, int nvars) 
{
	char** tmplist = NULL;
	int ntmp;

	newstringlist(&tmplist, &ntmp, NC_MAX_VARS);

	if( ncallvars(ncid1, tmplist, ntmp) )
			return 1;
	
	/* printf("%d: ncallvars returned.\n", __LINE__); */
	
	if( strlistu(tmplist, list, list, ntmp, nvars, nvars) )
			return 1;

	/* printf("%d: Variable names from file 1 collected.\n", __LINE__); */
	clearstringlist(tmplist, NC_MAX_VARS);
			
	if( ncallvars(ncid2, tmplist, ntmp) )
			return 1;

	if( strlistu(tmplist, list, list, ntmp, nvars, nvars) ) 
			return 1;

	/* printf("%d: Variable names from file 2 collected.\n", __LINE__); */
	freestringlist(&tmplist, ntmp);        

	return EXIT_SUCCESS;
}
/* *********************************************************** */

int 
nccmpformats(nccmpopts* opts)
{
	int status, fmt1, fmt2;

	status = nc_inq_format(ncid1, &fmt1);
	handle_error(status);

	status = nc_inq_format(ncid2, &fmt2);
	handle_error(status);
	
	if (fmt1 != fmt2) {
		fprintf(stderr, "DIFFER : FILE FORMATS : %s <> %s\n", NCFORMATSTR(fmt1), NCFORMATSTR(fmt2));
		
		if (!opts->warn[NCCMP_W_ALL] &&
			  !opts->warn[NCCMP_W_FORMAT])
			return EXIT_DIFFER;
	}
	
	return EXIT_SUCCESS;
}
/* *********************************************************** */

int 
nccmpglobalatts(nccmpopts* opts)
{
	int ngatts1, ngatts2, nattsex1, nattsex2, i, status, attid1, attid2, nprocessedatts;
	nc_type type1, type2;
	size_t len1, len2;
	char name1[NC_MAX_NAME], name2[NC_MAX_NAME];
	char** processedatts = NULL;
	char typestr1[1024], typestr2[1024];

	status = EXIT_SUCCESS;
	if (!opts->global)
		return status;

	if (opts->history == 0) 
		appendstringtolist(&opts->globalexclude, "history", &opts->nglobalexclude);
	
	/*	
	printf("globalexclude =");
	printstrlist(opts->globalexclude, opts->nglobalexclude, stdout);
	*/
	 	
	/* Number of global atts to compare with exclusion taken into account. */
	nattsex1 = 0;
	nattsex2 = 0;
	
	status = nc_inq_natts(ncid1, &ngatts1);
	handle_error(status);
	
	status = nc_inq_natts(ncid2, &ngatts2);
	handle_error(status);
	
	for(i = 0; i < ngatts1; ++i) {
		attid1 = i;
		status = nc_inq_attname(ncid1, NC_GLOBAL, attid1, name1);
		handle_error(status);
		
		if (!instringlist(opts->globalexclude, name1, opts->nglobalexclude)) {
			++nattsex1;
		}
	}
	
	for(i = 0; i < ngatts2; ++i) {
		attid2 = i;
		status = nc_inq_attname(ncid2, NC_GLOBAL, attid2, name2);
		handle_error(status);
		
		if (!instringlist(opts->globalexclude, name2, opts->nglobalexclude)) {
			++nattsex2;
		}
	}
	
	if(nattsex1 != nattsex2) {
		fprintf(stderr, "DIFFER : NUMBER OF GLOBAL ATTRIBUTES : %d <> %d\n", nattsex1, nattsex2);
		
		if (!opts->warn[NCCMP_W_ALL]) 
			status = EXIT_DIFFER;
			
		if(!opts->force) return status;
	}
		
	if(newstringlist(&processedatts, &i, NC_MAX_VARS)) {
		fprintf(stderr, "ERROR: Failed to allocated string list for comparing  global attributes.\n");
		return EXIT_FATAL;
	}
	
	for(i = 0; i < ngatts1; ++i) {
		attid1 = i;
		status = nc_inq_attname(ncid1, NC_GLOBAL, attid1, name1);
		handle_error(status);
		
		/* Log that this gatt was processed. */
		addstringtolist(processedatts, name1, NC_MAX_VARS);
		
		if (instringlist(opts->globalexclude, name1, opts->nglobalexclude)) 
			continue;
			
		status = nc_inq_att(ncid1, NC_GLOBAL, name1, &type1, &len1);
		if (status != NC_NOERR) {
			fprintf(stderr, "Query failed on global attribute in %s\n", opts->file1);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		status = nc_inq_att(ncid2, NC_GLOBAL, name1, &type2, &len2);
		if (status != NC_NOERR)	{
			fprintf(stderr, "DIFFER : NAME OF GLOBAL ATTRIBUTE : %s : GLOBAL ATTRIBUTE DOESN'T EXIST IN \"%s\"\n", name1, opts->file2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		if (type1 != type2)  {
			type2string(type1, typestr1);
			type2string(type2, typestr2);			
			fprintf(stderr, "DIFFER : GLOBAL ATTRIBUTE TYPES : %s : %s <> %s\n", name1, typestr1, typestr2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		if (len1 != len2)   {
			prettyprintatt(ncid1, NULL, NC_GLOBAL, name1, typestr1);
			prettyprintatt(ncid2, NULL, NC_GLOBAL, name1, typestr2);
			fprintf(stderr, "DIFFER : LENGTHS OF GLOBAL ATTRIBUTE : %s : %lu <> %lu : VALUES : %s <> %s\n", name1, 
							(unsigned long)len1, (unsigned long)len2, typestr1, typestr2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		if (cmpattval(ncid1,ncid2,NC_GLOBAL,NC_GLOBAL,name1,len1,type1) != NC_NOERR) {
			/* Pretty print values. */
			prettyprintatt(ncid1, NULL, NC_GLOBAL, name1, typestr1);
			prettyprintatt(ncid2, NULL, NC_GLOBAL, name1, typestr2);
			fprintf(stderr, "DIFFER : VALUES OF GLOBAL ATTRIBUTE : %s : %s <> %s\n", name1, typestr1, typestr2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
	}
	
	for(i = 0; i < ngatts2; ++i) {
		attid2 = i;
		status = nc_inq_attname(ncid2, NC_GLOBAL, attid2, name2);
		if (status != NC_NOERR) {
			fprintf(stderr, "Query failed for global attribute name in %s\n", opts->file2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return EXIT_DIFFER;
		}
		
		/* Skip if already processed (or excluded). */
		if (instringlist(processedatts, name2, NC_MAX_VARS))
			continue;
		
		/* Log that this att was processed. */
		addstringtolist(processedatts, name2, NC_MAX_VARS);
		
		status = nc_inq_att(ncid2, NC_GLOBAL, name2, &type2, &len2);
		if (status != NC_NOERR) {
			fprintf(stderr, "Query failed on global attribute in %s\n", opts->file2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		status = nc_inq_att(ncid1, NC_GLOBAL, name2, &type1, &len1);
		if (status != NC_NOERR) {
			fprintf(stderr, "DIFFER : NAME OF GLOBAL ATTRIBUTE : %s : GLOBAL ATTRIBUTE DOESN'T EXIST IN %s\n", name2, opts->file1);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		if (type1 != type2) {
			type2string(type1, typestr1);
			type2string(type2, typestr2);
			fprintf(stderr, "DIFFER : GLOBAL ATTRIBUTE TYPE : %s : %s <> %s\n", name1, typestr1, typestr2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		if (len1 != len2)  {
			fprintf(stderr, "DIFFER : LENGTHS OF GLOBAL ATTRIBUTE : %s : %lu <> %lu\n", name1, (unsigned long)len1, (unsigned long)len2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
		
		if (cmpattval(ncid1,ncid2,NC_GLOBAL,NC_GLOBAL,name1,len1,type1) != NC_NOERR) {
			/* Pretty print values. */
			prettyprintatt(ncid1, NULL, NC_GLOBAL, name1, typestr1);
			prettyprintatt(ncid2, NULL, NC_GLOBAL, name1, typestr2);
			fprintf(stderr, "DIFFER : VALUES OF GLOBAL ATTRIBUTE : %s : %s <> %s\n", name1, typestr1, typestr2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			if(opts->force) continue; else return status;
		}
	}
		
	/* Clear the list. */
	freestringlist(&processedatts, NC_MAX_VARS);
	processedatts = NULL;

	return status;
}
/* *********************************************************** */
int 
nccmpmetadata(nccmpopts *opts)
{
	int i, j, j1, j2, status, ncstatus, dimid1, dimid2, tmp1, tmp2, attid1, attid2, natts1, natts2;
	size_t len1, len2;
	char name1[NC_MAX_NAME], name2[NC_MAX_NAME], recname1[NC_MAX_NAME], recname2[NC_MAX_NAME], typestr1[1024], typestr2[1024];
	char** processedatts = NULL;

	status = EXIT_SUCCESS;

	if (opts->verbose)
		printf("INFO: Comparing metadata.\n");

	if (opts->verbose)
		printf("INFO: Comparing number of dimensions.\n");

	if (ndims1 != ndims2) {
		fprintf(stderr, "DIFFER : NUMBER OF DIMENSIONS IN FILES : %d <> %d\n", ndims1, ndims2);
		if (!opts->warn[NCCMP_W_ALL]) 
			status = EXIT_DIFFER;

		if (!opts->force)
			return status;
	}

	if (opts->verbose)
		printf("INFO: Getting record dimension names, if they exist.\n");

	if (recid1 != -1) {
	  ncstatus = nc_inq_dimname(ncid1, recid1, recname1);
	  handle_error(ncstatus);
	} else
	  strcpy(recname1, "");
	
	if (recid2 != -1) {
	  ncstatus = nc_inq_dimname(ncid2, recid2, recname2);
	  handle_error(ncstatus);
	} else
	  strcpy(recname1, "");
	
	/* Dimensions */
	if (opts->verbose)
		printf("INFO: Comparing dimension lengths.\n");

	for(i = 0; i < ndims1; ++i) {
		dimid1 = i;
		ncstatus = nc_inq_dim(ncid1, dimid1, name1, &len1);
		if (ncstatus != NC_NOERR) {
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			fprintf(stderr, "Failed to query dimension id %d in file %s.\n", dimid1, opts->file1);
			if(opts->force) continue; else return status;
		}

		ncstatus = nc_inq_dimid(ncid2, name1, &dimid2);
		if (ncstatus != NC_NOERR) {
			fprintf(stderr, "DIFFER : NAME : DIMENSION : %s : DIMENSION DOESN'T EXIST IN \"%s\"\n", name1, opts->file2);
			
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
			
			if(opts->force) continue; else return status;
		}

		ncstatus = nc_inq_dim(ncid2, dimid2, name2, &len2);
		if (ncstatus != NC_NOERR) {
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
				
			fprintf(stderr, "Failed to query dimension \"%s\" in file \"%s\".\n", name1, opts->file2);
			if(opts->force) continue; else return status;
		}

		if (len1 != len2) {
			fprintf(stderr, "DIFFER : LENGTHS : DIMENSION : %s : %lu <> %lu\n", name1, 
							(unsigned long)len1, (unsigned long)len2);
							
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;
			
			if(opts->force) continue; else return status;
		}
	}

	/* Variables */
	if (opts->verbose)
		printf("INFO: Comparing variable datatypes and rank.\n");

	for(i=0; i < opts->ncmpvarlist; ++i) {
		j1 = findvar(opts->cmpvarlist[i], vars1);
		if (j1 == -1) {
			fprintf(stderr, "DIFFER : NAME : VARIABLE : %s : VARIABLE DOESN'T EXIST IN \"%s\"\n", 	opts->cmpvarlist[i], opts->file1);
			
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;                                         
				
			if(opts->force) continue; else goto recover;
		}

		j2 = findvar(opts->cmpvarlist[i], vars2);
		if (j2 == -1) {
			fprintf(stderr, "DIFFER : NAME : VARIABLE : %s : VARIABLE DOESN'T EXIST IN \"%s\"\n", 	opts->cmpvarlist[i], opts->file2);
			
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;                                         
			
			if(opts->force) continue; else goto recover;
		}

		if(vars1[j1].type != vars2[j2].type) {
			type2string(vars1[j1].type, typestr1);
			type2string(vars2[j2].type, typestr2);                        
			fprintf(stderr, "DIFFER : TYPES : VARIABLE : %s : %s <> %s\n", opts->cmpvarlist[i], typestr1, typestr2);
			
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;             
			                            
			if(opts->force) continue; else goto recover;
		}

		if(vars1[j1].ndims != vars2[j2].ndims) {
			fprintf(stderr, "DIFFER : NUMBER : DIMENSIONS : VARIABLE : %s : %d <> %d\n", opts->cmpvarlist[i], ndims1, ndims2);
			
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;                                         
				
			if(opts->force) continue; else goto recover;
		}
	}
	
	/*printf("DEBUG : %d : \n", __LINE__);*/
	if (opts->verbose)
		printf("INFO: Comparing variables' dimension names.\n");

	for(i=0; i < opts->ncmpvarlist; ++i) {
		j1 = findvar(opts->cmpvarlist[i], vars1);
		j2 = findvar(opts->cmpvarlist[i], vars2);
		
		if ((j1 == -1) || (j2 == -1))
			continue;
		
		/* dimensions */
		for(j = 0; j < vars1[j1].ndims; ++j) {
			dimid1 = vars1[j1].dimids[j];
			
			if (j < vars2[j2].ndims)
				dimid2 = vars2[j2].dimids[j];
			else
				break;

			/*printf("DEBUG : %d : %s,  %s, %s\n", __LINE__, opts->cmpvarlist[i], dims1[dimid1].name, dims2[dimid2].name);*/
			
			if (strcmp(dims1[dimid1].name, dims2[dimid2].name) != 0) {
				fprintf(stderr, "DIFFER : DIMENSION NAMES FOR VARIABLE %s : %s <> %s\n", opts->cmpvarlist[i], dims1[dimid1].name, dims2[dimid2].name);
				
				if (!opts->warn[NCCMP_W_ALL]) 
					status = EXIT_DIFFER;
				
				if(opts->force) continue; else goto recover;				
			}

			tmp1 = strcmp(dims1[dimid1].name, recname1);
			tmp2 = strcmp(dims2[dimid2].name, recname2);

			if ( (tmp1 == 0) && (tmp2 != 0) ) {
				fprintf(stderr, "DIFFER : VARIABLE : %s : DIMENSION %s IS RECORD IN FILE \"%s\" BUT NOT IN \"%s\"\n", vars1[j1].name, dims1[dimid1].name, opts->file1, opts->file2);
				
				if (!opts->warn[NCCMP_W_ALL]) 
					status = EXIT_DIFFER;                                         
					
				if(opts->force) continue; else goto recover;                     
			} else if ( (tmp1 != 0) && (tmp2 == 0) ) {
				fprintf(stderr, "DIFFER : VARIABLE : %s : DIMENSION %s IS RECORD IN FILE \"%s\" BUT NOT IN \"%s\"\n", vars1[j1].name, dims2[dimid2].name, opts->file2, opts->file1);
				
				if (!opts->warn[NCCMP_W_ALL]) 
					status = EXIT_DIFFER;                                         
					
				if(opts->force) continue; else goto recover;                     
			}
		}
	}
	
	if (opts->verbose)
		printf("INFO: Comparing variables' attributes.\n");

	/*printf("DEBUG : %d : \n", __LINE__);*/
	/* Pass in 'i' as dummy. */
	if (newstringlist(&processedatts, &i, NC_MAX_VARS)) {
		fprintf(stderr, "ERROR: Failed to allocate string list for comparing attributes.\n");
		return EXIT_FATAL;
	}
	/*printf("DEBUG : %d : \n", __LINE__);*/
	
	for(i=0; i < opts->ncmpvarlist; ++i) {
		/*printf("DEBUG : %d : i = %d\n", __LINE__, i);*/
		j1 = findvar(opts->cmpvarlist[i], vars1);
		j2 = findvar(opts->cmpvarlist[i], vars2);
		
		if ((j1 == -1) || (j2 == -1))
			continue;

		natts1 = natts2 = 0;
		clearstringlist(processedatts, NC_MAX_VARS);
		
		/*printf("DEBUG : %d : var=%s\n", __LINE__, opts->cmpvarlist[i]);*/
		fflush(stdout);
		
		/* Attributes */
		for(attid1=0; attid1 < vars1[j1].natts; ++attid1) {
			ncstatus = nc_inq_attname(ncid1, vars1[j1].varid, attid1, name1);
			if (ncstatus != NC_NOERR)	{
				if (!opts->warn[NCCMP_W_ALL]) 
					status = EXIT_DIFFER;
					
				if(opts->force) continue; else goto recover;
			}
	
			if (instringlist(opts->excludeattlist, name1, opts->nexcludeatt) || instringlist(processedatts, name1, NC_MAX_VARS))
				continue;

			/* Log that this att was processed. */
			addstringtolist(processedatts, name1, NC_MAX_VARS);
			++natts1;
			/*printf("natts1 %s, %d\n", name1, natts1);*/
			ncstatus = cmpatt(vars1[j1].varid, vars2[j2].varid, name1, vars1[j1].name, opts);
			if (ncstatus == EXIT_DIFFER) {
				status = ncstatus;
				if(opts->force) continue; else goto recover;
			}
		}
		
		/*printf("DEBUG : %d : \n", __LINE__);*/
		for(attid2=0; attid2 < vars2[j2].natts; ++attid2) {
			ncstatus = nc_inq_attname(ncid2, vars2[j2].varid, attid2, name2);
			if (ncstatus != NC_NOERR) {
				fprintf(stderr, "Failed to query variable %s attribute in file \"%s\"\n", vars2[j2].name, opts->file2);
				
				if (!opts->warn[NCCMP_W_ALL]) 
					status = EXIT_DIFFER;
					
				if(opts->force) continue; else goto recover;
			}

			if (instringlist(opts->excludeattlist, name2, opts->nexcludeatt))
				continue;
			
			/* Count non-excluded attribute. */
			++natts2;
			/*printf("natts2 %s, %d\n", name2, natts2);*/
			if (instringlist(processedatts, name2, NC_MAX_VARS))
				continue;

			/* Log that this att was processed. */
			addstringtolist(processedatts, name2, NC_MAX_VARS);
			
			/* Do comparison. */
			ncstatus = cmpatt(vars1[j1].varid, vars2[j2].varid, name2, vars2[j2].name, opts);
			if (ncstatus == EXIT_DIFFER) {
				status = ncstatus;
				if(opts->force) continue; else goto recover;
			}
		}
		
		if(natts1 != natts2) {
			fprintf(stderr, "DIFFER : NUMBER OF ATTRIBUTES : VARIABLE : %s : %d <> %d\n", opts->cmpvarlist[i], natts1, natts2);
			if (!opts->warn[NCCMP_W_ALL]) 
				status = EXIT_DIFFER;                                         
				
			if(opts->force) continue; else goto recover;
		}
	}
	
 recover:
	freestringlist(&processedatts, NC_MAX_VARS);
	processedatts = NULL;
	
	return status;	
}
/* *********************************************************** */
/* Returns index into varstruct array if found using name otherwise -1. */
int findvar(char * name, varstruct *vars)
{
	int i;
	for(i=0; i < NC_MAX_VARS; ++i) {
		if (strcmp(name, vars[i].name) == 0)
			return i;
	}

	return -1;
}
/* *********************************************************** */
/* Element compare and return first offset that differs. 
	 Arrays should have sentinals at end that are guaranteed to differ.
*/
off_t cmp_text(char* in1, char* in2) {
	char const *p1, *p2;
	for(p1 = in1, p2 = in2; *p1 == *p2; ++p1, ++p2)
    continue;
	return p1 - in1;
}
off_t cmp_uchar(unsigned char* in1, unsigned char* in2) {
	unsigned char const *p1, *p2;
	for(p1 = in1, p2 = in2; *p1 == *p2; ++p1, ++p2)
    continue;
	return p1 - in1;
}
off_t cmp_short(short* in1, short* in2) {
	short const *p1, *p2;
	for(p1 = in1, p2 = in2; *p1 == *p2; ++p1, ++p2)
    continue;
	return p1 - in1;
}
off_t cmp_int(int* in1, int* in2) {
	int const *p1, *p2;
	for(p1 = in1, p2 = in2; *p1 == *p2; ++p1, ++p2)
    continue;
	return p1 - in1;
}
off_t cmp_float(float* in1, float* in2) {
	float const *p1, *p2;
	for(p1 = in1, p2 = in2; *p1 == *p2; ++p1, ++p2)
    continue;
	return p1 - in1;
}
off_t cmp_double(double* in1, double* in2) {
	double const *p1, *p2;
	for(p1 = in1, p2 = in2; *p1 == *p2; ++p1, ++p2)
    continue;
	return p1 - in1;
}

/* *********************************************************** */
/* Do the comparision of variables.
	 Record index (rec) is optional; -1 if not applicable.
	 Returns comparison result success or failure.
*/
int cmpvar(char* name, int rec, nccmpopts* opts)
{
	unsigned char *c1,*c2;
	short *s1,*s2;
	int *i1,*i2;
	float *f1,*f2;
	double *d1,*d2;
	varstruct *v1,*v2;
	int idx1, idx2, status, i;
	size_t start[NC_MAX_DIMS], count[NC_MAX_DIMS];
	char tmpstr1[256], tmpstr2[256], idxstr[256];
	off_t nitems, diff, cmplen;
	size_t odomax[NC_MAX_DIMS];
	int diffstatus = EXIT_SUCCESS;
	char *message = "DIFFER : VARIABLE : %s : POSITION : %s : VALUES : %s <> %s\n";
	char value1str[32], value2str[32];
	double doublevalue1, doublevalue2;
	int intvalue1, intvalue2;
	short shortvalue1, shortvalue2;
	float floatvalue1, floatvalue2;
	char textvalue1, textvalue2;
	unsigned char ucharvalue1, ucharvalue2;
	char printHex = strchr(opts->precision, 'X') || strchr(opts->precision, 'x');
	
	if (opts->verbose) {
		if (rec != -1)
			printf("INFO: Comparing data for variable \"%s\" at record %d.\n", name, (int)rec);
		else
			printf("INFO: Comparing non-record data for variable \"%s\".\n", name);
	}
	
	idx1 = findvar(name, vars1);
	idx2 = findvar(name, vars2);

	v1 = &vars1[idx1];
	v2 = &vars2[idx2];

	/*printf("DEBUG : %s len : %d <> %d\n", name, v1->len, v2->len); */
	
	if (v1->len != v2->len) {
		fprintf(stderr, "DIFFER : SIZE OF VARIABLE \"%s\" : %d <> %d\n", name, (int)v1->len, (int)v2->len);
		
		if (!opts->warn[NCCMP_W_ALL]) \
			return EXIT_DIFFER;
		else
			return EXIT_SUCCESS;
	}
	
	for(i=0; i < v1->ndims; ++i) {
		start[i] = 0;
		odomax[i] = v1->dimlens[i] - 1;
	}

#ifdef __DEBUG__
	printf("DEBUG : %d : odomax = ", __LINE__);
	for(i=0; i < v1->ndims; ++i) {
		printf("%d ", odomax[i]);
	}
	printf("\n");
#endif

	/* If has record dim. */
	if (v1->hasrec && (rec >= 0)) 
		start[0] = rec;
	
	/* Read in slab for last dimension at-a-time only. 
		We'll then loop over all the other outer dimensions. */
	for(i=0; i < v1->ndims-1; ++i) {
		count[i] = 1;
	}

   	/* We'll always read in entire last dimension
       except if only dimension is record. */
	if ((v1->ndims == 1)  && (v1->hasrec)) {
        nitems = 1;
    } else
        nitems = v1->dimlens[v1->ndims-1];
	
    count[v1->ndims-1] = nitems;
		
	/*printf("DEBUG : %d : nitems = %d\n", __LINE__, nitems);\*/

#define CMP_VAR(TYPE, NCFUNTYPE, P1, P2) {\
		P1 = (TYPE*)malloc(sizeof(TYPE) * (nitems + 1));      \
		P2 = (TYPE*)malloc(sizeof(TYPE) * (nitems + 1));      \
     \
		do { \
			/* printf("start = %d %d %d %d, count = %d %d %d %d\n", (int)start[0], (int)start[1], (int)start[2], (int)start[3], (int)count[0], (int)count[1], (int)count[2], (int)count[3]); */ \
			status = nc_get_vara_##NCFUNTYPE(ncid1, v1->varid, start, count, P1); \
			handle_error(status); \
			status = nc_get_vara_##NCFUNTYPE(ncid2, v2->varid, start, count, P2); \
			handle_error(status); \
			/* Sentinels. */ \
			P1[nitems] = 0; \
			P2[nitems] = 1; \
			\
			cmplen = nitems; \
            /* for(i=0; i<nitems; ++i) { \
                printf("nitems = %d, rec = %d, P1[%d] = %g, P2[%d] = %g\n", nitems, rec, i, P1[i], i, P2[i]); \
            } */ \
			diff = cmp_##NCFUNTYPE(P1, P2); \
			while (diff < cmplen) { \
				if (!opts->warn[NCCMP_W_ALL]) \
					diffstatus = EXIT_DIFFER; \
				else \
					diffstatus = EXIT_SUCCESS; \
				\
				if (opts->fortran) \
					getidxstr_fortran(v1, start, nitems-cmplen+diff, idxstr); \
				else \
					getidxstr(v1, start, nitems-cmplen+diff, idxstr); \
				\
				NCFUNTYPE##value1 = P1[nitems-cmplen+diff]; \
				NCFUNTYPE##value2 = P2[nitems-cmplen+diff]; \
				if (printHex) {\
					NCFUNTYPE##ToHex(NCFUNTYPE##value1, value1str); \
					NCFUNTYPE##ToHex(NCFUNTYPE##value2, value2str); \
				} else { \
					NCFUNTYPE##ToString(NCFUNTYPE##value1, value1str, opts->precision); \
					NCFUNTYPE##ToString(NCFUNTYPE##value2, value2str, opts->precision); \
				} \
				fprintf(stderr, message, v1->name, idxstr, value1str, value2str); \
				/* printf("%d nitems=%d, cmplen=%d, diff=%d\n", __LINE__, nitems, cmplen, diff); */\
				if (opts->force) { \
					cmplen = cmplen - (diff+1); \
					diff = cmp_##NCFUNTYPE(P1+diff+1, P2+diff+1); \
				} else  \
					goto break_##NCFUNTYPE; \
			} \
			/* Increment all but first (if record) and last dimensions. */\
		} while(odometer(start, odomax, (rec >= 0), v1->ndims-2)); \
		break_##NCFUNTYPE:\
		free(P1); \
		free(P2); \
		}

	switch(v1->type) {
	case NC_BYTE: 
		CMP_VAR(unsigned char, 
								uchar, 
								c1, c2);
		break;
		
	case NC_CHAR:
		CMP_VAR( char, 
								text, 
								c1, c2);
		break;
		
	case NC_SHORT:
		CMP_VAR(short, short, s1, s2)
		break;
		
	case NC_INT:
		CMP_VAR(int, int, i1, i2)
		break;
	
	case NC_FLOAT:
		CMP_VAR(float, float, f1, f2);
		break;
	
	case NC_DOUBLE:
		CMP_VAR(double, double, d1, d2);
		break;
	}

	return diffstatus;
}
/* *********************************************************** */
/* Do the comparision of variables.
	 Record index (rec) is optional; -1 if not applicable.
	 Returns comparison result success or failure.
*/
int cmpvartol(char* name, int rec, nccmpopts* opts)
{
	char *c1,*c2;
	short *s1,*s2;
	int *i1,*i2;
	float *f1,*f2;
	double *d1,*d2;
	varstruct *v1,*v2;
	int idx1, idx2, status, i;
	size_t start[NC_MAX_DIMS], count[NC_MAX_DIMS];
	char tmpstr1[256], tmpstr2[256], idxstr[256];
	off_t nitems, diff, cmplen;
	size_t odomax[NC_MAX_DIMS];
	int diffstatus = EXIT_SUCCESS;
	char *message = "DIFFER : VARIABLE : %s : POSITION : %s : VALUES : %s <> %s : PERCENT : %g\n";
	char value1str[32], value2str[32];
	double absdelta;
	double doublevalue1, doublevalue2;
	int intvalue1, intvalue2;
	short shortvalue1, shortvalue2;
	float floatvalue1, floatvalue2;
	char textvalue1, textvalue2;
	unsigned char ucharvalue1, ucharvalue2;
	char printHex = strchr(opts->precision, 'X') || strchr(opts->precision, 'x');
	
	if (opts->verbose) {
		if (rec != -1)
			printf("INFO: Comparing data for variable \"%s\" at record %d.\n", name, (int)rec);
		else
			printf("INFO: Comparing non-record data for variable \"%s\".\n", name);
	}

	idx1 = findvar(name, vars1);
	idx2 = findvar(name, vars2);

	if (idx1 < 0) {
		if (! opts->metadata) /* This gets reported in cmpmeta. */
			fprintf(stderr, "DIFFER : Failed to find variable \"%s\" in file \"%s\".\n", name, opts->file1);
		
		if (!opts->warn[NCCMP_W_ALL])
			return EXIT_DIFFER;
		else
			return EXIT_SUCCESS;
	}

	if (idx2 < 0) {
		if (! opts->metadata) /* This gets reported in cmpmeta. */
			fprintf(stderr, "DIFFER : Failed to find variable \"%s\" in file \"%s\".\n", name, opts->file2);
	
		if (!opts->warn[NCCMP_W_ALL])
			return EXIT_DIFFER;
		else
			return EXIT_SUCCESS;
	}

	v1 = &vars1[idx1];
	v2 = &vars2[idx2];
		
	for(i=0; i < v1->ndims; ++i) {
		start[i] = 0;
		odomax[i] = v1->dimlens[i] - 1;
	}

	/* If has record dim. */
	if (v1->hasrec && (rec >= 0)) 
		start[0] = rec;
	
	/* Read in slab for last dimension at-a-time only. 
		We'll then loop over all the other outer dimensions. */
	for(i=0; i < v1->ndims-1; ++i) {
		count[i] = 1;
	}
	
   	/* We'll always read in entire last dimension
       except if only dimension is record. */
	if ((v1->ndims == 1)  && (v1->hasrec)) {
        nitems = 1;
    } else
        nitems = v1->dimlens[v1->ndims-1];
	
    count[v1->ndims-1] = nitems;

    /* todo: make cmpvar and cmpvartol same function to re-use code immediately above; just use conditional to choose CMP_VAR or CMP_VARTOL macro. */
#define CMP_VARTOL(TYPE, NCFUNTYPE, P1, P2) {				\
		P1 = (TYPE*)malloc(sizeof(TYPE) * nitems);      \
		P2 = (TYPE*)malloc(sizeof(TYPE) * nitems);      \
     \
		do { \
            /* printf("start = %d %d %d %d, count = %d %d %d %d\n", (int)start[0], (int)start[1], (int)start[2], (int)start[3], (int)count[0], (int)count[1], (int)count[2], (int)count[3]); */ \
			status = nc_get_vara_##NCFUNTYPE(ncid1, v1->varid, start, count, P1); \
			handle_error(status); \
			status = nc_get_vara_##NCFUNTYPE(ncid2, v2->varid, start, count, P2); \
			handle_error(status); \
			\
            /* for(i=0; i<nitems; ++i) { \
                printf("nitems = %d, rec = %d, P1[%d] = %g, P2[%d] = %g\n", nitems, rec, i, P1[i], i, P2[i]); \
            } */ \
			for(i=0; i < nitems; ++i)	\
			{												\
				absdelta = fabs((double)(P1[i]-P2[i]));			\
				if (opts->abstolerance ? (absdelta > opts->tolerance) : (double)absdelta*100./(fabs((double)P1[i]) > fabs((double)P2[i]) ? fabs((double)P1[i]) : fabs((double)P2[i])) > opts->tolerance)	\
				{														\
					if (!opts->warn[NCCMP_W_ALL]) \
						diffstatus = EXIT_DIFFER; \
					else \
						diffstatus = EXIT_SUCCESS; \
						\
					if ((v1->ndims == 1)  && (v1->hasrec)) {\
						if (opts->fortran) \
							getidxstr_fortran(v1, start, rec, idxstr); \
						else \
							getidxstr(v1, start, rec, idxstr); \
					} else {\
						if (opts->fortran) \
							getidxstr_fortran(v1, start, i, idxstr); \
						else \
							getidxstr(v1, start, i, idxstr); \
					}\
  				NCFUNTYPE##value1 = P1[i]; \
  				NCFUNTYPE##value2 = P2[i]; \
					if (printHex) {\
						NCFUNTYPE##ToHex(NCFUNTYPE##value1, value1str); \
						NCFUNTYPE##ToHex(NCFUNTYPE##value2, value2str); \
				  } else {\
						NCFUNTYPE##ToString(NCFUNTYPE##value1, value1str, opts->precision); \
						NCFUNTYPE##ToString(NCFUNTYPE##value2, value2str, opts->precision); \
				  } \
					fprintf(stderr, message, v1->name, idxstr, value1str, value2str, (double)absdelta*100./(fabs((double)P1[i]) > fabs((double)P2[i]) ? fabs((double)P1[i]) : fabs((double)P2[i])) ); \
					if (! opts->force)						 					\
					{                              					\
						goto break_##NCFUNTYPE;										\
					}																				\
				}																					\
			}																						\
		} while(odometer(start, odomax, (rec >= 0), v1->ndims-2)); \
		break_##NCFUNTYPE:\
		free(P1); \
		free(P2); \
}

	switch(v1->type) {
	case NC_BYTE: 
		CMP_VARTOL(unsigned char, 
								uchar, 
								c1, c2);
		break;
		
	case NC_CHAR:
		CMP_VARTOL( char, 
								text, 
								c1, c2);
		break;
		
	case NC_SHORT:
		CMP_VARTOL(short, short, s1, s2)
		break;
		
	case NC_INT:
		CMP_VARTOL(int, int, i1, i2)
		break;
	
	case NC_FLOAT:
		CMP_VARTOL(float, float, f1, f2);
		break;
	
	case NC_DOUBLE:
		CMP_VARTOL(double, double, d1, d2);
		break;
	}

	return diffstatus;
}
/* *********************************************************** */
int 
nccmpdatarecvartol(char* varname, nccmpopts* opts,
								size_t recstart, size_t recend)
{
    int cmpstatus;
    int status = EXIT_SUCCESS;
  
    for(; recstart <= recend; ++recstart)
    {
			cmpstatus = cmpvartol(varname, recstart, opts) || status;
			if (cmpstatus != EXIT_SUCCESS) {
				status = cmpstatus;
				if (opts->force)
					continue;
				else
					break;
			}
    }

    return status;
}
/* *********************************************************** */
int 
nccmpdatarecvar(char* varname, nccmpopts* opts,
								size_t recstart, size_t recend)
{
    int cmpstatus;
    int status = EXIT_SUCCESS;
  
    for(; recstart <= recend; ++recstart)
    {
			cmpstatus = cmpvar(varname, recstart, opts) || status;
			if (cmpstatus != EXIT_SUCCESS) {
				status = cmpstatus;
				if (opts->force)
					continue;
				else
					break;
			}
    }

    return status;
}
/* *********************************************************** */
int nccmpdatatol(nccmpopts* opts)
{
	int i, idx1, idx2;
	int status, cmpstatus, nprocessed;
	char** processed = NULL;

	status = EXIT_SUCCESS;

	if (opts->verbose)
		printf("INFO: Comparing data with tolerance.\n");
	
	if(newstringlist(&processed, &nprocessed, NC_MAX_VARS)) {
		fprintf(stderr, "ERROR: Failed to allocated string list for comparing data.\n");
		return EXIT_FATAL;
	}
	
	for(i=0; i < opts->ncmpvarlist; ++i) {
		if (instringlist(processed, opts->cmpvarlist[i], nprocessed))
			/* Skip varnames already processed. */
			continue;
		
		if (opts->verbose)
			printf("INFO: Comparing data for variable \"%s\".\n", opts->cmpvarlist[i]);

		addstringtolist(processed, opts->cmpvarlist[i], nprocessed);

		/* Has rec? */
		idx1 = findvar(opts->cmpvarlist[i], vars1);
		if (vars1[idx1].hasrec) {	

			/* Compare only if # recs are equal and not zero. */ 
			if ((nrec1 == nrec2) && (nrec1+nrec2)) {
				if (cmpstatus = nccmpdatarecvartol(opts->cmpvarlist[i], opts, 0, nrec1-1)) {
					status = cmpstatus;
					if (opts->force)
						continue;
					else
						break;
				}
			}
		} else {
			if (cmpstatus = cmpvartol(opts->cmpvarlist[i], -1, opts)) {
				status = cmpstatus;
				if (opts->force)
					continue;
				else
					break;
			}
		}
	}
	
	if (opts->verbose)
		printf("INFO: Finished comparing data.\n");

	return status;
}
/* *********************************************************** */
int nccmpdata(nccmpopts* opts)
{
	int i, idx1, idx2;
	int status, cmpstatus, nprocessed;
	char** processed = NULL;
    char str1[32], str2[32];
    
	status = EXIT_SUCCESS;

	if (opts->verbose)
		printf("INFO: Comparing data.\n");

	if (opts->tolerance != 0)
		return nccmpdatatol(opts);
	
	if(newstringlist(&processed, &nprocessed, NC_MAX_VARS)) {
		fprintf(stderr, "ERROR: Failed to allocated string list for comparing data.\n");
		return EXIT_FATAL;
	}
	
	for(i=0; i < opts->ncmpvarlist; ++i) {
		if (instringlist(processed, opts->cmpvarlist[i], nprocessed))
			/* Skip varnames already processed. */
			continue;
		
		if (opts->verbose)
			printf("INFO: Comparing data for variable \"%s\".\n", opts->cmpvarlist[i]);

		addstringtolist(processed, opts->cmpvarlist[i], nprocessed);
        
		idx1 = findvar(opts->cmpvarlist[i], vars1);	
    idx2 = findvar(opts->cmpvarlist[i], vars2);	

    	if (idx1 < 0) {
	    	if (! opts->metadata) /* This gets reported in cmpmeta. */
	    		fprintf(stderr, "DIFFER : Failed to find variable \"%s\" in file \"%s\".\n", opts->cmpvarlist[i], opts->file1);
	    	
				if (!opts->warn[NCCMP_W_ALL])
					status = EXIT_DIFFER;
			if (opts->force)
    			continue;
           	else
    			break;
	    }

	    if (idx2 < 0) {
 	    	if (! opts->metadata) /* This gets reported in cmpmeta. */
			    fprintf(stderr, "DIFFER : Failed to find variable \"%s\" in file \"%s\".\n", opts->cmpvarlist[i], opts->file2);

				if (!opts->warn[NCCMP_W_ALL])
					status = EXIT_DIFFER;
			if (opts->force)
    			continue;
           	else
    			break;
	    }
                
        if (vars1[idx1].len != vars2[idx2].len) {
		    fprintf(stderr, "DIFFER : SIZE OF VARIABLE \"%s\" : %d <> %d\n", opts->cmpvarlist[i], (int)vars1[idx1].len, (int)vars2[idx2].len);

				if (!opts->warn[NCCMP_W_ALL])
					status = EXIT_DIFFER;
			if (opts->force)
    			continue;
           	else
    			break;
	    }
        
        if (vars1[idx1].type != vars2[idx2].type) {
            type2string(vars1[idx1].type, str1);
            type2string(vars2[idx2].type, str2);
		    fprintf(stderr, "DIFFER : TYPE OF VARIABLE \"%s\" : %s <> %s\n", opts->cmpvarlist[i], str1, str2);
		
			if (!opts->warn[NCCMP_W_ALL])
		    status = EXIT_DIFFER;

			if (opts->force)
    			continue;
           	else
    			break;
	    }
        
        /* Has rec? */
		if (vars1[idx1].hasrec) {	

			/* Compare only if # recs are equal and not zero. */ 
			if ((nrec1 == nrec2) && (nrec1+nrec2)) {
				if (cmpstatus = nccmpdatarecvar(opts->cmpvarlist[i], opts, 0, nrec1-1)) {
					status = cmpstatus;
					if (opts->force)
						continue;
					else
						break;
				}
			}
		} else {
			if (cmpstatus = cmpvar(opts->cmpvarlist[i], -1, opts)) {
				status = cmpstatus;
				if (opts->force)
					continue;
				else
					break;
			}
		}
	}
	
	if (opts->verbose)
		printf("INFO: Finished comparing data.\n");

	return status;
}
/* *********************************************************** */
int 
nccmp(nccmpopts* opts)
{
	int status, status2;
	
	if (opts->verbose)
		printf("INFO: Opening input files.\n");

	status = openfiles(opts);
	if (status)
		return status;

	if (opts->verbose)
		printf("INFO: Creating variable comparison list.\n");
	
	status = makecmpvarlist(opts);
	if (status)
		return status;
	
	if (opts->verbose)
		printf("INFO: Comparing file formats.\n");

	status = nccmpformats(opts);
	if (status && !opts->force)
		return status;

	if (opts->verbose)
		printf("INFO: Comparing global attributes.\n");
	
	if(status2 = nccmpglobalatts(opts))
		status = status2;
		
	if (status && !opts->force)
		return status;
	
	if (opts->verbose)
		printf("INFO: Collecting dimension information for first file.\n");

	getdiminfo(ncid1, dims1, &ndims1);

	if (opts->verbose)
		printf("INFO: Collecting dimension information for second file.\n");
	
	getdiminfo(ncid2, dims2, &ndims2);

	if (opts->verbose)
		printf("INFO: Collecting variable information for first file.\n");
	
	getvarinfo(ncid1, vars1, &nvars1);

	if (opts->verbose)
		printf("INFO: Collecting variable information for second file.\n");
	
	getvarinfo(ncid2, vars2, &nvars2);

	if(status2 = nccmprecinfo(opts))
		status = status2;
		
	if (status && !opts->force)
		return status;
		
	if (opts->metadata) {
		if(status2 = nccmpmetadata(opts))
			status = status2;
	}
	
	if (status && !opts->force)
			return status;
			
	if (opts->data) {
		if(status2 = nccmpdata(opts))
			status = status2;
	}

	if (opts->verbose)
		printf("INFO: Comparisons complete. Freeing memory.\n");
	
 	 /* printf("DEBUG: %d : status = %d\n", __LINE__, status); */
	
	return status;
}

/* *********************************************************** */

int
main(int argc, char** argv)
{
	int status;
	nccmpopts opts;

	status = EXIT_SUCCESS;

	initnccmpopts(&opts);
	
	/* parse command-line args. & options */
	status = getnccmpopts(argc, argv, &opts);
	
	if (status != EXIT_SUCCESS) 
	  goto end;
	
	if (opts.verbose)
		printf("INFO: Command-line options parsed.\n");

	status = nccmp(&opts);
	
end:
	freenccmpopts(&opts);
	
	return status;
} 
