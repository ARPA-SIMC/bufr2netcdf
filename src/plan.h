/*
 * plan - BUFR to NetCDF conversion strategy
 *
 * Copyright (C) 2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef B2NC_PLAN_H
#define B2NC_PLAN_H

//#include "namer.h"
#include "valarray.h"
//#include <wreport/varinfo.h>
//#include <string>
#include <vector>
//#include <map>
#include <cstdio>

namespace wreport {
struct Bulletin;
}

namespace b2nc {
struct Options;
struct ValArray;
struct NCOutfile;

namespace plan {

struct Section;

/// Strategy for encoding one BUFR variable
struct Variable
{
    /**
     * Data valarray for this variable.
     *
     * Can be NULL if we are a placeholder for a replicated subsection, or if
     * this variable should be skipped.
     */
    ValArray* data;

    /**
     * QBits valarray for this variable.
     *
     * Can be NULL if there are no QBits for this variable.
     */
    ValArray* qbits;

    /**
     * If data is NULL, we are a placeholder for a replicated subsection.
     *
     * In that case, this is non-NULL and points to the replicated subsection
     * whose R replicator operator is found at this position.
     *
     * It can be that data is NULL and subsection is NULL in case this is a
     * variable that should just be skipped.
     */
    Section* subsection;

    Variable();
    ~Variable();

    void print(FILE* out);

private:
    // Forbid copy
    Variable(const Variable&);
    Variable& operator=(const Variable&);
};

/**
 * Strategy for how to encode a sequence of BUFR variables.
 *
 * A Section does not contain replicated sequences, but every replicated
 * sequence has its own Section. Sections contain pointers to replicated
 * sequences in the positions where the R operator is found.
 */
struct Section
{
    size_t id;
    std::vector<Variable*> entries;

    /**
     * If we are a replicated section using delayed replication, store here
     * information about the loop and delayed replication count.
     */
    LoopInfo loop;

    /**
     * Cursor used when iterating the structure mapping decoded data to the
     * plan
     */
    unsigned cursor;

    Section(size_t id);
    ~Section();

    // Get the variable pointed by the cursor
    Variable& current() const;
    // Get the idx-th variable after the cursor
    Variable& current(unsigned idx) const;

    void define(NCOutfile& outfile);
    void putvar(NCOutfile& outfile) const;
    void print(FILE* out) const;
    void print(FILE* out, unsigned first, unsigned count=0) const;

private:
    // Forbid copy
    Section(const Section&);
    Section& operator=(const Section&);
};

}

/**
 * BUFR to NetCDF Conversion stragegy
 *
 * The plan is built analysing the Data Description Section (DDS) of the first
 * BUFR seen. It is assumed that a different plan is built for every different
 * DDS.
 *
 * The plan structure follows the structure of the BUFR DDS as decoded by
 * wreport, where qbits are found in variable attributes.
 *
 * The BUFR DDS is split into numbered sections, corresponding to replicated
 * sequences. Section 0 contains all descriptors that are outside of
 * replicators.
 */
struct Plan
{
    const Options& opts;
    std::vector<plan::Section*> sections;
    /**
     * Varinfo describing qbits variables.
     *
     * This is stored here to guarantee it the same lifetime as the plan.
     */
    wreport::Varinfo qbits_info;

    Plan(const Options& opts);
    ~Plan();

    /// Create a new section
    plan::Section& create_section();
    /// get a section. only used during tests. returns NULL if not found
    const plan::Section* get_section(unsigned section) const;
    /// get an array. only used during tests. returns NULL if not found
    const plan::Variable* get_variable(unsigned section, unsigned pos) const;

    void build(const wreport::Bulletin& bulletin);
    void define(NCOutfile& outfile);
    void putvar(NCOutfile& outfile) const;

    void print(FILE* out) const;

private:
    // Forbid copy
    Plan(const Plan&);
    Plan& operator=(const Plan&);
};

}

#endif

