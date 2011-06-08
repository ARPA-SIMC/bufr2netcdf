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

#include "plan.h"
#include "valarray.h"
//#include "utils.h"
//#include "mnemo.h"
#include <wreport/var.h>
#include <wreport/bulletin.h>
//#include <netcdf.h>
#include <stack>
#include <cstring>

using namespace wreport;
using namespace std;

namespace b2nc {

namespace plan {

Variable::Variable()
    : data(0), qbits(0), subsection(0)
{
}

Variable::~Variable()
{
    if (data) delete data;
    if (qbits) delete qbits;
}

void Variable::print(FILE* out)
{
    if (data)
    {
        fprintf(out, "Data(%s)", data->name.c_str());
        if (qbits)
            fprintf(out, " Qbits(%s)", qbits->name.c_str());
    } else {
        fprintf(out, "subsection %zd", subsection->id);
    }
    putc('\n', out);
}


Section::Section(size_t id) : id(id) {}
Section::~Section()
{ 
    for (vector<Variable*>::iterator i = entries.begin();
            i != entries.end(); ++i)
        delete *i;
}

void Section::print(FILE* out)
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        fprintf(stderr, "%zd: ", i);
        entries[i]->print(out);
    }
}

}

namespace {

struct PlanMaker : opcode::Explorer
{
    struct Section
    {
        // Section we are editing
        plan::Section& section;

        // True if
        bool has_qbits;

        Section(plan::Section& section)
            : section(section), has_qbits(false)
        {
        }

        void add_data(Varinfo info)
        {
            // TODO if (toplevel)
                // TODO return ValArray::make_singlevalarray(type, info);
            // TODO else
                // TODO arr.reset(ValArray::make_multivalarray(type, info, i->second));
            section.entries.push_back(new plan::Variable);
            plan::Variable& pi = *section.entries.back();
            pi.data = ValArray::make_singlevalarray(Namer::DT_DATA, info);
            if (has_qbits)
                pi.qbits = ValArray::make_singlevalarray(Namer::DT_QBITS, info);
            // TODO fill in things like names
        }

        void add_subplan(plan::Section& subplan)
        {
            section.entries.push_back(new plan::Variable);
            plan::Variable& pi = *section.entries.back();
            pi.subsection = &subplan;
        }
    };

    // Plans we are building
    Plan& plan;

    // Plan stack. The current plan ID is on top of the stack
    stack<Section> current_plan;

    // B table used for varcode resolution
    const Vartable* btable;

    PlanMaker(Plan& plan, const Bulletin& b)
        : plan(plan), btable(b.btable)
    {
        current_plan.push(Section(plan.create_section()));
    }

    ~PlanMaker()
    {
    }

    void b_variable(Varcode code)
    {
        Section& s = current_plan.top();
        Varinfo info = btable->query(code);
        s.add_data(info);
    }

    void c_modifier(Varcode code)
    {
        switch (WR_VAR_X(code))
        {
            case 4:
                // Toggle has_qbits flag
                current_plan.top().has_qbits = WR_VAR_Y(code) != 0;
                break;
            case 5:
                // TODO: current plan.push_back(ValArray(code))
                break;
        }
    }

    void r_replication_begin(Varcode code)
    {
        Section& s = current_plan.top();
        plan::Section& ns = plan.create_section();
        current_plan.push(Section(ns));
        s.add_subplan(ns);
    }

    void r_replication_end(Varcode code)
    {
        current_plan.pop();
    }
};

}


Plan::Plan() {}

Plan::~Plan()
{
    for (vector<plan::Section*>::iterator i = sections.begin();
            i != sections.end(); ++i)
        delete *i;
}

void Plan::build(const wreport::Bulletin& bulletin)
{
    PlanMaker pm(*this, bulletin);
    bulletin.explore_datadesc(pm);
}

plan::Section& Plan::create_section()
{
    sections.push_back(new plan::Section(sections.size()));
    return *sections.back();
}

void Plan::print(FILE* out)
{
    for (size_t i = 0; i < sections.size(); ++i)
    {
        fprintf(stderr, "Section %zd:\n", i);
        sections[i]->print(stderr);
    }
}



}
