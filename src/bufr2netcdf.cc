#include "convert.h"
#include "options.h"
#include <wreport/error.h>
#include <string>
#include <cstdio>

#include "config.h"

#ifdef HAS_GETOPT_LONG
#include <getopt.h>
#endif

using namespace b2nc;
using namespace wreport;
using namespace std;

void usage(FILE* out)
{
    fprintf(out, "Usage: bufr2netcdf [options] file[s]\n");
    fprintf(out, "Convert BUFR files to NetCDF according to COSMO conventions\n");
    fprintf(out, "For each input file it generates one or more output files,\n");
    fprintf(out, " one for each different BUFR type encountered.\n");
    fprintf(out, "\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -h, --help                  this help message.\n");
    fprintf(out, "  -v, --verbose               verbose output.\n");
    fprintf(out, "  -D, --debug                 debug output.\n");
    fprintf(out, "  -o PFX, --outfile=PFX       prefix to use for output files.\n");
    fprintf(out, "  -n                          generate variable names in the form\n");
    fprintf(out, "                              Type_FXXYYY_RRR instead of using a mnemonic.\n");
#ifndef HAS_GETOPT_LONG
    fprintf(out, "NOTE: long options are not supported on this system\n");
#endif

}

int main(int argc, char* argv[])
{
#ifdef HAS_GETOPT_LONG
    static struct option long_options[] =
    {
        /* These options set a flag. */
        {"help",    no_argument,       NULL, 'h'},
        {"outfile", required_argument, NULL, 'o'},
        {"verbose", no_argument,       NULL, 'v'},
        {"debug",   no_argument,       NULL, 'D'},
        {0, 0, 0, 0}
    };
#endif

    Options options;

    while (1)
    {
        /* `getopt_long' stores the option index here. */
        int option_index = 0;

#ifdef HAS_GETOPT_LONG
        int c = getopt_long(argc, argv, "o:vhnD",
                long_options, &option_index);
#else
        int c = getopt(argc, argv, "o:vhnD");
#endif

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                usage(stdout);
                return 0;
            case 'o':
                options.out_fname = optarg;
                break;
            case 'n':
                options.use_mnemonic = false;
                break;
            case 'D':
                options.debug = true;
                // debug includes verbose, so fall through into it
            case 'v':
                options.verbose = true;
                break;
            default:
                // getopt already prints an error message
                fputc('\n', stderr);
                usage(stderr);
                return 1;
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Usage: %s [-o file] file1 [file2 [file3 ..]]\n", argv[0]);
        return 1;
    }

    if (options.out_fname.empty())
    {
        options.out_fname = argv[optind];
        options.out_fname += ".nc";
    }

    try {
        Dispatcher dispatcher(options);

        while (optind < argc)
        {
            if (options.verbose) fprintf(stderr, "Reading from %s\n", argv[optind]);
            read_bufr(argv[optind++], dispatcher);
        }

        dispatcher.close();
    } catch (std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
