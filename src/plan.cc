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
#include <wreport/vartable.h>
#include <wreport/bulletin.h>
#include <wreport/bulletin/interpreter.h>
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

struct PlanMaker : bulletin::Interpreter
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

        unique_ptr<ValArray> make_array(Namer::DataType type, Varinfo name_info, Varinfo type_info)
        {
            string name;
            string mnemo;
            unsigned rcnt = maker.namer->name(type, name_info->code, section.id, name, mnemo);
            unique_ptr<ValArray> arr;
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

        unique_ptr<ValArray> make_data_array(Varinfo info)
        {
            unique_ptr<ValArray> arr = make_array(Namer::DT_DATA, info, info);

            // Add references information to arr
            for (map<Varcode, const ValArray*>::const_iterator i = context.begin();
                    i != context.end(); ++i)
                arr->references.push_back(*i);

            return arr;
        }

        unique_ptr<ValArray> make_qbits_array(Varinfo info, ValArray& master)
        {
            unique_ptr<ValArray> arr = make_array(Namer::DT_QBITS, info, maker.plan.qbits_info);
            arr->master = &master;
            master.slaves.push_back(arr.get());
            return arr;
        }

        unique_ptr<ValArray> make_char_array(Varcode code)
        {
            Varinfo info = maker.tables.get_chardata(code, WR_VAR_Y(code));
            unique_ptr<ValArray> arr = make_array(Namer::DT_CHAR, info, info);
            return arr;
        }

        void add_data(Varinfo info)
        {
            /**
             * Some context code invalidate others, and we need to do it right
             * away because the invalidation takes effect also for themselves
             */
            switch (info->code)
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
            if (has_qbits && WR_VAR_F(info->code) == 0 && WR_VAR_X(info->code) != 31)
                pi.qbits = make_qbits_array(info, *pi.data).release();

            // Update current context information
            if (WR_VAR_F(info->code) == 0 && WR_VAR_X(info->code) < 10)
            {
                Varcode code = info->code;
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

    // Namer used to give names to variables
    Namer* namer;

    // Index used in numbering loops for naming in NetCDF attributes
    unsigned loop_index;

    /**
     * Current value of string length override from C08 modifiers (0 for no
     * override)
     */
    int c_string_len_override;

    PlanMaker(Plan& plan, const Bulletin& b, const Options& opts)
        : Interpreter(b.tables, b.datadesc),
          plan(plan),
          namer(Namer::get(opts).release()),
          loop_index(0)
    {
        current_plan.push(Section(*this, plan.create_section()));
    }

    ~PlanMaker()
    {
        delete namer;
    }

    void add_var_to_plan(Varinfo info, const char* desc)
    {
        Section& s = current_plan.top();

        // Deal with local extensions that are not present in our tables
        if (WR_VAR_F(info->code) == 0 && WR_VAR_Y(info->code) >= 192)
            if (!tables.btable->contains(info->code))
                return;

        s.add_data(info);

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: add %s ", WR_VAR_FXY(info->code), desc);
            current_plan.top().section.entries.back()->print(stderr);
        }
    }

    void define_variable(Varinfo info) override
    {
        add_var_to_plan(info, "variable");
    }

    unsigned define_delayed_replication_factor(Varinfo) override
    {
        return 1;
    }

    void define_raw_character_data(Varcode code) override
    {
        Section& s = current_plan.top();
        s.add_char(code);

        if (plan.opts.debug)
        {
            fprintf(stderr, "%d%02d%03d: add character data variable\n", WR_VAR_FXY(code));
        }
    }

    void c_modifier(Varcode code, Opcodes& next) override
    {
        switch (WR_VAR_X(code))
        {
            case 4: {
                /*
                 * Add associated field.
                 *
                 * Precede each data element with Y bits of information. This
                 * operation associates a data field (e.g. quality control
                 * information) of Y bits with each data element.
                 *
                 * The Add Associated Field operator, whenever used, must be
                 * immediately followed by the Class 31 Data description operator
                 * qualifier 0 31 021 to indicate the meaning of the associated
                 * fields.
                 */
                unsigned nbits = WR_VAR_Y(code);
                Varcode sig_code = (nbits && !next.empty()) ? next.pop_left() : 0;
                // Toggle has_qbits flag
                current_plan.top().has_qbits = nbits != 0;
                if (sig_code)
                    add_var_to_plan(get_varinfo(sig_code), "associated field significance");

                if (plan.opts.debug)
                {
                    if (sig_code)
                        fprintf(stderr, "%d%02d%03d: add significance variable %d\n", WR_VAR_FXY(code), sig_code);
                    else
                        fprintf(stderr, "%d%02d%03d: exit scope of associated field\n", WR_VAR_FXY(code));
                }
                break;
            }
            case 6: {
                /*
                 * Signify data width for the immediately following local
                 * descriptor.
                 *
                 * Y bits of data are described by the immediately following
                 * descriptor.
                 */
                Varcode desc_code = !next.empty() ? next.pop_left() : 0;
                // Length of next local descriptor
                if (unsigned nbits = WR_VAR_Y(code))
                {
                    if (tables.btable->contains(desc_code))
                    {
                        Varinfo info = tables.btable->query(desc_code);
                        if (info->bit_len == nbits)
                            add_var_to_plan(info, "local descriptor found in our B table");
                    }

                    if (plan.opts.debug)
                    {
                        fprintf(stderr, "%d%02d%03d: add possibly unsupported variable %d%02d%03d\n",
                                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code),
                                WR_VAR_F(desc_code), WR_VAR_X(desc_code), WR_VAR_Y(desc_code));
                    }
                }
                break;
            }
            default:
                bulletin::Interpreter::c_modifier(code, next);
                break;
        }
    }

    void r_replication(Varcode code, Varcode delayed_code, const Opcodes& ops) override
    {
        plan::Section& ns = plan.create_section();
        ns.loop.index = loop_index++;

        if (delayed_code)
        {
            if (plan.opts.debug)
            {
                fprintf(stderr, "%d%02d%03d: add replication variable %d%02d%03d\n",
                    WR_VAR_FXY(code), WR_VAR_FXY(delayed_code));
            }

            define_variable(get_varinfo(delayed_code));
            ns.loop.var = current_plan.top().section.entries.back()->data;
        }

        Section& s = current_plan.top();
        current_plan.push(Section(*this, ns));
        s.add_subplan(ns);

        if (plan.opts.debug)
            fprintf(stderr, "%d%02d%03d: add section\n", WR_VAR_FXY(code));

        // Init subplan context with a fork of the parent context
        current_plan.top().context = s.context;
        current_plan.top().has_qbits = s.has_qbits;

        Interpreter::r_replication(code, delayed_code, ops);

        if (plan.opts.debug)
            fprintf(stderr, "%d%02d%03d: end of section\n", WR_VAR_FXY(code));

        current_plan.pop();
    }

    unsigned define_bitmap_delayed_replication_factor(Varinfo) override
    {
        return 0;
    }

    void define_bitmap(unsigned) override
    {
    }

    //void r_bitmap(Varcode code, Varcode delayed_code, const Opcodes& ops) override
    //{
    //    // Skip the section if it's just defining a bitmap
    //}

    void run_r_repetition(unsigned cur, unsigned total) override
    {
        if (cur > 0) return;
        Interpreter::run_r_repetition(cur, total);
    }

    void define_c03_refval_override(Varcode) override
    {
        // Nothing to do, it should get handled in the decoder
    }


private:
    // Forbid copy
    PlanMaker(const PlanMaker&);
    PlanMaker& operator=(const PlanMaker&);
};

}


Plan::Plan(const Options& opts) : opts(opts)
{
    // qbits_info.set_binary(WR_VAR(0, 33, 0), "Q-BITS FOR FOLLOWING VALUE", 32);
    qbits_info=varinfo_create_bufr(WR_VAR(0, 33, 0), "Q-BITS FOR FOLLOWING VALUE", "CODE TABLE", 32);
}

Plan::~Plan()
{
    for (vector<plan::Section*>::iterator i = sections.begin();
            i != sections.end(); ++i)
        delete *i;
    varinfo_delete(std::move(qbits_info));
}

void Plan::build(const wreport::Bulletin& bulletin)
{
    PlanMaker pm(*this, bulletin, opts);
    pm.run();
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
        fprintf(out, "Section %zu:\n", i);
        sections[i]->print(out);
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
