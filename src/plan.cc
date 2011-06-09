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
#include "options.h"
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


Section::Section(size_t id) : id(id), loop_var(0), loop_var_index(0), cursor(0) {}
Section::~Section()
{ 
    for (vector<Variable*>::iterator i = entries.begin();
            i != entries.end(); ++i)
        delete *i;
}

Variable& Section::current() const
{
    if (cursor >= entries.size())
        error_consistency::throwf("trying to read section %zd past its end (%u/%zd)",
                id, cursor, entries.size());
    return *entries[cursor];
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
        PlanMaker& maker;

        // Section we are editing
        plan::Section& section;

        // True if
        bool has_qbits;

        Section(PlanMaker& maker, plan::Section& section)
            : maker(maker), section(section), has_qbits(false)
        {
        }

        auto_ptr<ValArray> make_array(Namer::DataType type, Varinfo info)
        {
            string name;
            string mnemo;
            char TODO_temporary_tag[20];
            snprintf(TODO_temporary_tag, 20, "%zd", section.id);
            unsigned rcnt = maker.namer->name(type, info->var, TODO_temporary_tag, name, mnemo);
            auto_ptr<ValArray> arr(ValArray::make_singlevalarray(type, info));
            arr->name = name;
            arr->mnemo = mnemo;
            arr->rcnt = rcnt;
            arr->type = type;

            // TODO if (toplevel)
                // TODO return ValArray::make_singlevalarray(type, info);
            // TODO else
                // TODO arr.reset(ValArray::make_multivalarray(type, info, i->second));


            return arr;
        }

        auto_ptr<ValArray> make_data_array(Varinfo info)
        {
            return make_array(Namer::DT_DATA, info);
        }

        auto_ptr<ValArray> make_qbits_array(Varinfo info, ValArray& master)
        {
            auto_ptr<ValArray> arr = make_array(Namer::DT_QBITS, info);
            arr->master = &master;
            master.slaves.push_back(arr.get());
            return arr;
        }

        void add_data(Varinfo info)
        {
            section.entries.push_back(new plan::Variable);
            plan::Variable& pi = *section.entries.back();
            pi.data = make_data_array(info).release();
            if (has_qbits)
                pi.qbits = make_qbits_array(info, *pi.data).release();
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

    // Namer used to give names to variables
    Namer* namer;

    // Index used in numbering loops for naming in NetCDF attributes
    unsigned loop_var_index;

    PlanMaker(Plan& plan, const Bulletin& b, const Options& opts)
        : plan(plan),
          btable(b.btable),
          namer(Namer::get(opts).release()),
          loop_var_index(0)
    {
        current_plan.push(Section(*this, plan.create_section()));
    }

    ~PlanMaker()
    {
        delete namer;
    }

    void b_variable(Varcode code)
    {
        Section& s = current_plan.top();

        // Deal with local extensions that are not present in our tables
        if (WR_VAR_F(code) == 0 && WR_VAR_Y(code) >= 192)
            if (!btable->contains(code))
                return;

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

    void r_replication_begin(Varcode code, Varcode delayed_code)
    {
        plan::Section& ns = plan.create_section();

        if (delayed_code)
        {
            b_variable(delayed_code);
            ns.loop_var = current_plan.top().section.entries.back();
            ns.loop_var_index = loop_var_index++;
        }

        Section& s = current_plan.top();
        current_plan.push(Section(*this, ns));
        s.add_subplan(ns);
    }

    void r_replication_end(Varcode code)
    {
        current_plan.pop();
    }

private:
    // Forbid copy
    PlanMaker(const PlanMaker&);
    PlanMaker& operator=(const PlanMaker&);
};

}


Plan::Plan(const Options& opts) : opts(opts) {}

Plan::~Plan()
{
    for (vector<plan::Section*>::iterator i = sections.begin();
            i != sections.end(); ++i)
        delete *i;
}

void Plan::build(const wreport::Bulletin& bulletin)
{
    PlanMaker pm(*this, bulletin, opts);
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
