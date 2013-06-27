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
#include "ncoutfile.h"
#include "utils.h"
//#include "mnemo.h"
#include <wreport/var.h>
#include <wreport/bulletin.h>
#include <netcdf.h>
#include <stack>
#include <map>
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


Section::Section(size_t id) : id(id), cursor(0) {}
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

Variable& Section::current(unsigned idx) const
{
    if (cursor + idx >= entries.size())
        error_consistency::throwf("trying to read section %zd past its end (%u/%zd)",
                id, cursor + idx, entries.size());
    return *entries[cursor + idx];
}

void Section::define(NCOutfile& outfile)
{
    for (vector<plan::Variable*>::iterator i = entries.begin();
            i != entries.end(); ++i)
    {
        Variable& v = **i;
        if (v.data) v.data->define(outfile);
        if (v.qbits) v.qbits->define(outfile);
    }
}

void Section::putvar(NCOutfile& outfile) const
{
    for (vector<plan::Variable*>::const_iterator i = entries.begin();
            i != entries.end(); ++i)
    {
        Variable& v = **i;
        if (v.data) v.data->putvar(outfile);
        if (v.qbits) v.qbits->putvar(outfile);
    }
}

void Section::print(FILE* out) const
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        fprintf(stderr, "%zd: ", i);
        entries[i]->print(out);
    }
}

void Section::print(FILE* out, unsigned first, unsigned count) const
{
    if (count == 0) count = entries.size();
    for (size_t i = first; i < entries.size() && i < first + count; ++i)
    {
        fprintf(stderr, "%zd: ", i);
        entries[i]->print(out);
    }
}

}

namespace {

/**
 * RAII-style temporarily override a variable value for the duration of the
 * scope
 */
template<typename T>
struct ValueOverride
{
    T& val;
    T old_val;

    ValueOverride(T& val, T new_val)
        : val(val), old_val(val)
    {
        val = new_val;
    }
    ~ValueOverride()
    {
        val = old_val;
    }
};

struct PlanMaker : opcode::Visitor
{
    struct Section
    {
        PlanMaker& maker;

        // Section we are editing
        plan::Section& section;

        // True if qbits are expected at this point of decoding
        bool has_qbits;

        // Currently active context information
        map<Varcode, const ValArray*> context;

        // Previously seen varcode (0 at start of subset or if the previous varcode
        // had xx >= 10)
        Varcode prev_code;
        // Number of times in a row the previous varcode appeared
        unsigned prev_code_count;


        Section(PlanMaker& maker, plan::Section& section)
            : maker(maker), section(section), has_qbits(false), prev_code(0), prev_code_count(0)
        {
        }

        auto_ptr<ValArray> make_array(Namer::DataType type, Varinfo name_info, Varinfo type_info)
        {
            string name;
            string mnemo;
            unsigned rcnt = maker.namer->name(type, name_info->var, section.id, name, mnemo);
            auto_ptr<ValArray> arr;
            if (section.id == 0)
                arr.reset(ValArray::make_singlevalarray(type, type_info));
            else
                arr.reset(ValArray::make_multivalarray(type, type_info, section.loop));
            arr->name = name;
            arr->mnemo = mnemo;
            arr->rcnt = rcnt;
            arr->type = type;

            return arr;
        }

        auto_ptr<ValArray> make_data_array(Varinfo info)
        {
            auto_ptr<ValArray> arr = make_array(Namer::DT_DATA, info, info);

            // Add references information to arr
            for (map<Varcode, const ValArray*>::const_iterator i = context.begin();
                    i != context.end(); ++i)
                arr->references.push_back(*i);

            return arr;
        }

        auto_ptr<ValArray> make_qbits_array(Varinfo info, ValArray& master)
        {
            auto_ptr<ValArray> arr = make_array(Namer::DT_QBITS, info, maker.qbits_info);
            arr->master = &master;
            master.slaves.push_back(arr.get());
            return arr;
        }

        auto_ptr<ValArray> make_char_array(Varcode code)
        {
            MutableVarinfo info = MutableVarinfo::create_singleuse();
            info->set_string(code, "C05YYY character data", WR_VAR_Y(code));
            auto_ptr<ValArray> arr = make_array(Namer::DT_CHAR, info, info);
            return arr;
        }

        void add_data(Varinfo info)
        {
            /**
             * Some context code invalidate others, and we need to do it right
             * away because the invalidation takes effect also for themselves
             */
            switch (info->var)
            {
                case WR_VAR(0, 4, 24):
                    context.erase(WR_VAR(0, 4, 25));
                    break;
                case WR_VAR(0, 4, 25):
                    for (int i = 0; i < 4; ++i)
                        context.erase(WR_VAR(i, 4, 24));
                    break;
            }

            section.entries.push_back(new plan::Variable);
            plan::Variable& pi = *section.entries.back();
            pi.data = make_data_array(info).release();
            if (has_qbits && WR_VAR_F(info->var) == 0 && WR_VAR_X(info->var) != 31)
                pi.qbits = make_qbits_array(info, *pi.data).release();

            // Update current context information
            if (WR_VAR_F(info->var) == 0 && WR_VAR_X(info->var) < 10)
            {
                Varcode code = info->var;
                if (prev_code == code)
                {
                    // Encode the repetition count in F
                    // 2 consecutive context variables)
                    code |= WR_VAR(prev_code_count, 0, 0);

                    // Prevent roll over after 3 times: in those cases we just
                    // overwrite the 4th, but the maximum we should see is really 2
                    // in a row
                    if (prev_code_count < 3)
                        ++prev_code_count;
                }
                else
                {
                    prev_code = code;
                    prev_code_count = 1;
                }
                context[code] = pi.data;
            } else {
                prev_code = 0;
                prev_code_count = 0;
            }
        }

        void add_subplan(plan::Section& subplan)
        {
            section.entries.push_back(new plan::Variable);
            plan::Variable& pi = *section.entries.back();
            pi.subsection = &subplan;
        }

        void add_char(Varcode code)
        {
            section.entries.push_back(new plan::Variable);
            plan::Variable& pi = *section.entries.back();
            pi.data = make_char_array(code).release();
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
    unsigned loop_index;

    // Varinfo describing qbits variables
    MutableVarinfo qbits_info;

    bool expect_bitmap;

    /// Current value of scale change from C modifier
    int c_scale_change;

    /// Current value of width change from C modifier
    int c_width_change;

    /**
     * Current value of string length override from C08 modifiers (0 for no
     * override)
     */
    int c_string_len_override;

    PlanMaker(Plan& plan, const Bulletin& b, const Options& opts)
        : plan(plan),
          btable(b.btable),
          namer(Namer::get(opts).release()),
          loop_index(0),
          qbits_info(MutableVarinfo::create_singleuse()),
          expect_bitmap(false),
          c_scale_change(0), c_width_change(0), c_string_len_override(0)
    {
        qbits_info->set(WR_VAR(0, 33, 0), "Q-BITS FOR FOLLOWING VALUE", "CODE TABLE", 0, 0, 10, 0, 32);
        current_plan.push(Section(*this, plan.create_section()));
    }

    ~PlanMaker()
    {
        delete namer;
    }

    /**
     * Return the Varinfo describing the variable \a code, possibly altered
     * taking into account current C modifiers
     */
    Varinfo get_varinfo(Varcode code)
    {
        Varinfo peek = btable->query(code);

        if (!c_scale_change && !c_width_change && !c_string_len_override)
            return peek;

        int scale = peek->scale;
        if (c_scale_change)
            scale += c_scale_change;

        int bit_len = peek->bit_len;
        if (peek->is_string() && c_string_len_override)
            bit_len = c_string_len_override * 8;
        else if (c_width_change)
            bit_len += c_width_change;

        return btable->query_altered(code, scale, bit_len);
    }

    void c_change_data_width(Varcode code, int change)
    {
        c_width_change = change;
    }

    void c_change_data_scale(Varcode code, int change)
    {
        c_scale_change = change;
    }

    void c_char_data_override(Varcode code, unsigned new_length)
    {
        c_string_len_override = new_length;
    }

    void b_variable(Varcode code)
    {
        Section& s = current_plan.top();

        // Deal with local extensions that are not present in our tables
        if (WR_VAR_F(code) == 0 && WR_VAR_Y(code) >= 192)
            if (!btable->contains(code))
                return;

        Varinfo info = get_varinfo(code);
        s.add_data(info);

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: add variable ", WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
            current_plan.top().section.entries.back()->print(stderr);
        }
    }

    void c_quality_information_bitmap(Varcode code) { expect_bitmap = true; }

    void c_substituted_value_bitmap(Varcode code) { expect_bitmap = true; }

    void c_associated_field(Varcode code, Varcode sig_code, unsigned nbits)
    {
        // Toggle has_qbits flag
        current_plan.top().has_qbits = WR_VAR_Y(code) != 0;
        if (sig_code)
            b_variable(sig_code);

        if (plan.opts.debug)
        {
            if (sig_code)
                fprintf(stderr, "%d%02d%03d: add significance variable %d\n", WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code), sig_code);
            else
                fprintf(stderr, "%d%02d%03d: exit scope of associated field\n", WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        }
    }

    void c_char_data(Varcode code)
    {
        Section& s = current_plan.top();
        s.add_char(code);

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: add character data variable\n", WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        }
    }

    void c_local_descriptor(Varcode code, Varcode desc_code, unsigned nbits)
    {
        if (btable->contains(desc_code))
        {
            Varinfo info = btable->query(desc_code);
            if (info->bit_len == WR_VAR_Y(code))
                b_variable(desc_code);
        }

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: add possibly unsupported variable %d%02d%03d\n",
                    WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code),
                    WR_VAR_F(desc_code), WR_VAR_X(desc_code), WR_VAR_Y(desc_code));
        }
    }

    void r_replication(Varcode code, Varcode delayed_code, const Opcodes& ops)
    {
        if (expect_bitmap)
        {
            // Skip the section if it's just defining a bitmap
            expect_bitmap = false;
            return;
        }

        plan::Section& ns = plan.create_section();
        ns.loop.index = loop_index++;

        if (delayed_code)
        {
            if (plan.opts.debug)
            {
                fprintf(stderr, "%d%02d%03d: add replication variable %d%02d%03d\n",
                    WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code),
                    WR_VAR_F(delayed_code), WR_VAR_X(delayed_code), WR_VAR_Y(delayed_code));
            }

            b_variable(delayed_code);
            ns.loop.var = current_plan.top().section.entries.back()->data;
        }

        Section& s = current_plan.top();
        current_plan.push(Section(*this, ns));
        s.add_subplan(ns);

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: add section\n",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        }

        // Init subplan context with a fork of the parent context
        current_plan.top().context = s.context;
        current_plan.top().has_qbits = s.has_qbits;

        ops.visit(*this);

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: end of section\n",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        }

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
    bulletin.visit_datadesc(pm);
}

plan::Section& Plan::create_section()
{
    sections.push_back(new plan::Section(sections.size()));
    return *sections.back();
}

const plan::Section* Plan::get_section(unsigned section) const
{
    if (section >= sections.size())
        return NULL;
    return sections[section];
}

const plan::Variable* Plan::get_variable(unsigned section, unsigned pos) const
{
    if (const plan::Section* s = get_section(section))
        if (pos < s->entries.size())
            return s->entries[pos];
    return NULL;
}

void Plan::print(FILE* out) const
{
    for (size_t i = 0; i < sections.size(); ++i)
    {
        fprintf(stderr, "Section %zd:\n", i);
        sections[i]->print(stderr);
    }
}

void Plan::define(NCOutfile& outfile)
{
    for (vector<plan::Section*>::iterator i = sections.begin();
            i != sections.end(); ++i)
        (*i)->define(outfile);
}

void Plan::putvar(NCOutfile& outfile) const
{
    for (vector<plan::Section*>::const_iterator i = sections.begin();
            i != sections.end(); ++i)
        (*i)->putvar(outfile);
}


}
