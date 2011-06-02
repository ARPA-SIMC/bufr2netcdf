#include "convert.h"
#include <wreport/error.h>
#include <string>
#include <cstdio>
#include <getopt.h>

using namespace b2nc;
using namespace wreport;
using namespace std;

int main(int argc, char* argv[])
{
    static struct option long_options[] =
    {
        /* These options set a flag. */
        {"outfile", required_argument, NULL, 'o'},
        {"verbose", no_argument,       NULL, 'v'},
        {0, 0, 0, 0}
    };

    bool verbose = false;
    Options options;

    while (1)
    {
        /* `getopt_long' stores the option index here. */
        int option_index = 0;

        int c = getopt_long(argc, argv, "o:v",
                long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 'o':
                options.out_fname = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            default:
                error_consistency::throwf("unknown option character %c (%d)", c, c);
                break;
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
            if (verbose) fprintf(stderr, "Reading from %s\n", argv[optind]);
            read_bufr(argv[optind++], dispatcher);
        }

        dispatcher.close();
    } catch (std::exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
