#include "tests.h"
#include <wreport/utils/testrunner.h>
#include <wreport/utils/term.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <exception>

static void signal_to_exception(int)
{
    throw std::runtime_error("killing signal catched");
}

int main(int, const char**)
{
    using namespace wreport::tests;

    signal(SIGSEGV, signal_to_exception);
    signal(SIGILL, signal_to_exception);

    auto& tests = TestRegistry::get();

    wreport::term::Terminal output(stderr);

    std::unique_ptr<FilteringTestController> controller;

    bool verbose = (bool)getenv("TEST_VERBOSE");

    if (verbose)
        controller.reset(new VerboseTestController(output));
    else
        controller.reset(new SimpleTestController(output));

#ifndef WREPORT_VERSION_MAJOR
#define allowlist whitelist
#define blocklist blacklist
#endif

    if (const char* allowlist = getenv("TEST_WHITELIST"))
        controller->allowlist = allowlist;

    if (const char* allowlist = getenv("TEST_ONLY"))
        controller->allowlist = allowlist;

    if (const char* blocklist = getenv("TEST_BLACKLIST"))
        controller->blocklist = blocklist;

    if (const char* blocklist = getenv("TEST_EXCEPT"))
        controller->blocklist = blocklist;

    auto all_results = tests.run_tests(*controller);
    TestResultStats rstats(all_results);
    rstats.print_results(output);
    if (verbose) rstats.print_stats(output);
    rstats.print_summary(output);
    return rstats.success ? 0 : 1;
}
