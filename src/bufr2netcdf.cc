#include "convert.h"
#include <cstdio>

using namespace b2nc;

int main(int argc, const char* argv[])
{
    Outfile outfile(argv[1]);

    for (int i = 2; i < argc; ++i)
        outfile.add_bufr(argv[i]);

    outfile.close();

    return 0;
}
