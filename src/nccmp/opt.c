/*
   Copyright (C) 2004-2006,2010 Remik Ziemlinski <first d0t surname att n0aa d0t g0v>

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

#include "opt.h"

static struct option const long_options[] =
{
        {"Attribute",   required_argument,      0, 'A'},
				/*{"cartesian_axis", required_argument, 0, 'c'},*/
        {"data",        no_argument,            0, 'd'},
        {"debug",       no_argument,            0, 'D'},
        {"exclude",     required_argument,      0, 'x'},
        {"force",       no_argument,            0, 'f'},
        {"fortran",     no_argument,            0, 'F'},        
        {"global",      no_argument,            0, 'g'},        
        {"globalex",    required_argument,      0, 'G'},        
        {"history",     no_argument,            0, 'h'},        
        {"metadata",    no_argument,            0, 'm'}, 
        {"precision",   required_argument,      0, 'p'},       
        {"quiet",       no_argument,            0, 'q'},
				{"tolerance",   required_argument,      0, 't'},
				{"Tolerance",   required_argument,      0, 'T'},
        {"variable",    required_argument,      0, 'v'},
        {"verbose",     no_argument,            0, 'b'},
        {"help",        no_argument,            0, 'H'},
        {"usage",       no_argument,            0, 'H'},
        {"version",     no_argument,            0, 'V'},
        {"warn",        required_argument,      0, 'w'},
        {0, 0, 0, 0}
};

void freenccmpopts(nccmpopts* popts)
{
	freestringlist(&popts->excludeattlist, popts->nexcludeatt);
	freestringlist(&popts->globalexclude, popts->nglobalexclude);
	freestringlist(&popts->excludelist, popts->nexclude);
	freestringlist(&popts->variablelist, popts->nvariable);
	freestringlist(&popts->cmpvarlist, popts->ncmpvarlist);
	XFREE(popts->file1);
	XFREE(popts->file2); 
	XFREE(popts->precision);              
}
void initnccmpopts(nccmpopts* popts)
{
        if (popts == NULL) return;
        
        popts->data     = 0;
        popts->debug 	= 0;
        popts->force    = 0;  
        popts->fortran  = 0;  
        popts->global   = 0;        
        popts->history  = 0;                
        popts->metadata = 0;  
        popts->precision = NULL;      
        popts->quiet    = 0;        
        popts->verbose  = 0;        
        popts->help     = 0;        
				popts->tolerance= 0;
				popts->abstolerance= 0;
        popts->version  = 0;        
        popts->nexclude = 0;        
        popts->nvariable = 0;
				popts->exclude  = 0;        
        popts->variable = 0;        
        popts->nexcludeatt = 0;        
				popts->excludeattlist = NULL;
				popts->excludelist = NULL;
				popts->variablelist = NULL;
        popts->file1    = NULL;        
        popts->file2    = NULL;        
				popts->nglobalexclude = 0;
				popts->globalexclude = NULL;
				popts->cmpvarlist = NULL;
				popts->ncmpvarlist = 0;
				
				memset(popts->warn, 0, NCCMP_W_NUMTAGS);
}
void printusage()
{
        fprintf(stderr, USAGE);
}
void printversion()
{
	fprintf(stderr, "nccmp %s\n", VERSION);							
	fprintf(stderr, "Built with NetCDF %s\n", nc_inq_libvers());
	fprintf(stderr, "Copyright (C) 2004-2007,2009,2010 Remik Ziemlinski\n	\
\n\
This program comes with NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of this program\n\
under the terms of the GNU General Public License.\n"); 
}
int getnccmpopts(int argc, char** argv, nccmpopts* popts)
{
	int c;       
	char** warningtags = NULL;
	int nwarningtags, i;
	char** valid_warnings = NULL;
	char tmp[256];
	
	/* Create searchable list of defined warning tags. 
	   'nwarningtags' is used as a dummy arg here. */
	newstringlist(&valid_warnings, &nwarningtags, NCCMP_W_NUMTAGS);
	strcpy(tmp, NCCMP_W_TAGS); /* Necessary evil to prevent segfault. */
	getstringlist(tmp, &valid_warnings, &nwarningtags); 
	/* printf("%s:%d\n", __FILE__, __LINE__); */
	
	if (popts == NULL) return EXIT_FATAL;
	
	if (newstringlist(&popts->globalexclude, &popts->nglobalexclude, NC_MAX_VARS) != EXIT_SUCCESS) {
		printf("ERROR: Failed to allocate memory for global attribute exclusion list.\n");
		exit(EXIT_FATAL);
	}
	
	if (newstringlist(&popts->excludeattlist, &popts->nexcludeatt, NC_MAX_VARS) != EXIT_SUCCESS) {
		printf("ERROR: Failed to allocate memory for attribute exclusion list.\n");
		exit(EXIT_FATAL);
	}

	if (newstringlist(&popts->excludelist, &popts->nexclude, NC_MAX_VARS) != EXIT_SUCCESS) {
		printf("ERROR: Failed to allocate memory for variable exclusion list.\n");
		exit(EXIT_FATAL);
	}
	
	if (newstringlist(&popts->variablelist, &popts->nvariable, NC_MAX_VARS) != EXIT_SUCCESS) {
		printf("ERROR: Failed to allocate memory for variable list.\n");
		exit(EXIT_FATAL);
	}

	while ( (c = getopt_long(argc, argv, "A:dx:fFghmp:qt:T:v:bHVG:w:", long_options, 0))
					!= -1
				)
	switch (c)
	{
	case 'd':
					popts->data = 1;
					break;

	case 'x':
					popts->exclude = 1;
					getstringlist(optarg, &popts->excludelist, &popts->nexclude);
					break;

	case 'f':
					popts->force = 1;
					break;

	case 'F':
					popts->fortran = 1;
					break;

	case 'g':
					popts->global = 1;        
					break;

	case 'h':
					popts->history = 1;
					break;

	case 'm':
					popts->metadata = 1;
					break;

	case 'p':
					popts->precision = XMALLOC(char,strlen(optarg) + 1); 
					strcpy(popts->precision, optarg);
	case 'q':
					popts->quiet = 1;
					break;
	case 't':
		popts->tolerance = strtod(optarg, NULL);
		popts->abstolerance = 1;
		if ( (errno == ERANGE) || (errno == EDOM))
			{
				fprintf(stderr, "ERROR: Specified tolerance cannot be used.\n");
				return EXIT_FATAL;
			}						
		break;
	case 'T':
		popts->tolerance = strtod(optarg, NULL);
		popts->abstolerance = 0;
		if ( (errno == ERANGE) || (errno == EDOM))
			{
				fprintf(stderr, "ERROR: Specified tolerance cannot be used.\n");
				return EXIT_FATAL;
			}
		break;
	case 'v':
		getstringlist(optarg, &popts->variablelist, &popts->nvariable);
		popts->variable = 1;       
		break;
	case 'b':
		popts->verbose = 1;
		break;
					
	case 'V':
		popts->version = 1;
		break;

	case ':':
		fprintf(stderr, "Error, -%c without argument.\n\n", optopt); 
		popts->help = 1;
		break;
		
	case '?':
		fprintf(stderr, "Error, Unknown argument %c.\n\n", optopt);        
		popts->help = 1;
		break;
		
	case 'H':
		popts->help = 1;
		break;
		
	case 'A':
		getstringlist(optarg, &popts->excludeattlist, &popts->nexcludeatt);
		break;
		
	case 'G':
		getstringlist(optarg, &popts->globalexclude, &popts->nglobalexclude);
		break;
		
	case 'w':
		newstringlist(&warningtags, &nwarningtags, NCCMP_W_NUMTAGS);
		getstringlist(optarg, &warningtags, &nwarningtags);
		
		for(i=0; i < nwarningtags; ++i) {
			if (instringlist(valid_warnings, warningtags[i], NCCMP_W_NUMTAGS) != 1) {
				/* Warn user of any invalid warning tags they supplied. */
				fprintf(stderr, "WARNING: Warning tag \"%s\" is unsupported and will be ignored.\n", warningtags[i]);
			}
		}
		
		/* Look for defined tags that are supported. */
		if (instringlist(warningtags, "all", nwarningtags))
			popts->warn[NCCMP_W_ALL] = 1;
		else if (instringlist(warningtags, "format", nwarningtags))
			popts->warn[NCCMP_W_FORMAT] = 1;
					
		break;
	}
	if (popts->help)
	{
		printusage();
		printversion();
		return EXIT_FATAL;
	}       
	if (popts->version)
	{
		printversion();
		return EXIT_FATAL;
	}
	if (optind == argc)
	{
		fprintf(stderr, "Error, missing operand after `%s'.\n\n", argv[argc - 1]);
		printusage();
		return EXIT_FATAL;
	} 
	if (! (popts->data || popts->metadata) )
	{
		fprintf(stderr, "Error, must supply at least one of these options: -d, -m.\n\n");
		printusage();
		return EXIT_FATAL;
	}
	if ( popts->variable && popts->exclude )
	{
		fprintf(stderr, "Error, cannot combine -x and -v options.\n\n");
		printusage();
		return EXIT_FATAL;
	}
	if (popts->precision == NULL) {
		popts->precision = XMALLOC(char,strlen("%g") + 1); 
		strcpy(popts->precision, "%g");
	}
	
	/* get filename arguments */   
	argc -= optind;
	argv += optind;
	if (    (argc < 2) ||
					(argv[0] == NULL) ||
					(argv[1] == NULL) 
			)
	{
		fprintf(stderr, "Error, 2 file arguments required.\n\n");
		printusage();
		popts->file1 = NULL;
		popts->file2 = NULL;
		return EXIT_FATAL;
	} else
	{
		/* store filenames */
		popts->file1 = XMALLOC(char,strlen(argv[0]) + 1);
		popts->file2 = XMALLOC(char,strlen(argv[1]) + 1); 
		strcpy(popts->file1, argv[0]);
		strcpy(popts->file2, argv[1]);                
	}

	if (popts->history && !popts->global)
	{
		fprintf(stderr, "Error, -g required for -h option.\n\n");
		printusage();
		return EXIT_FATAL;
	}
	if (!popts->metadata && (popts->history || popts->global) )
	{
		fprintf(stderr, "Error, -m required for -g and -h options.\n\n");
		printusage();
		return EXIT_FATAL;
	}
	
 	freestringlist(&warningtags, nwarningtags);
 	freestringlist(&valid_warnings, NCCMP_W_NUMTAGS);
	/*printf("%s:%d\n", __FILE__, __LINE__);*/
	
	return EXIT_SUCCESS;
}
