#include <algorithm>
#include "database.h"
#include "disjvars.h"
#include "io.h"
#include "parse.h"
#include "proof/analyze.h"
#include "syntaxiom.h"
#include "typecode.h"
#include "util.h"
#include "util/for.h"
#include "util/iter.h"
#include "util/progress.h"
#include "util/timer.h"

// Map syntax axioms.
void Syntaxioms::_map()
{
    FOR (const_reference axiom, *this)
    {
        // set of constants in the syntax axiom.
        std::set<strview> const & constants(axiom.second.constants);
        // Find the constant whose vector is shortest.
        typedef M_map_type::mapped_type Minvec;
        Minvec * pminvec(NULL);

        FOR (strview str, constants)
        {
            // *iter2 = a constant in the set.
            Minvec & vec(m_map[str]);
            // Update shortest vector and its size.
            if (!pminvec || vec.size() < pminvec->size())
            {
                pminvec = &vec;
                if (vec.empty())
                    break; // New constant found.
            }
        }
        // Add the syntax axiom.
        if (!pminvec) // No constant in the syntax axiom.
            pminvec = &m_map[""];
        pminvec->push_back(&axiom);
    }
}

// Filter assertions for syntax axioms, i.e.,
// those starting with a primitive type code and having no essential hypothesis
// and find their var types.
Syntaxioms::Syntaxioms
    (Assertions const & assertions, class Database const & database)
{
//std::cout << "Syntax axioms:" << std::endl;
    for (Assiter iter(assertions.begin()); iter != assertions.end(); ++iter)
    {
        if ((iter->second.type & Assertion::AXIOM) == 0)
            continue; // Skip theorems.

        Expression const & exp(iter->second.expression);
        if (unexpected(exp.empty(), "Empty expression", iter->first))
            continue;
        // Check if it starts with a primitive type code and has no essential hyps.
        if (database.typecodes().isprimitive(exp[0]) == 1 &&
            iter->second.hypcount() == iter->second.varcount())
        {
            Syntaxiom & syntaxiom = (*this)[iter->first.c_str];
            syntaxiom.assiter = iter;
            // Fill constants of the syntax axiom.
            std::remove_copy_if(exp.begin() + 1, exp.end(),
                                end_inserter(syntaxiom.constants), isvar);
//std::cout << iter->first << ' ';
        }
    }
//std::cout << std::endl;
    _map();
}

void Syntaxioms::addfromvec
    (std::set<strview> const & expconstants,
     std::vector<const_pointer> const * pv)
{
    if (!pv) return;

    FOR (const_pointer p, *pv)
    {
//std::cout << p->first << ' ';
        std::set<strview> const & constants(p->second.constants);
        // If the constants in the syntax axiom are all found in exp,
        if (std::includes(expconstants.begin(), expconstants.end(),
                          constants.begin(), constants.end()))
        {
            // add it to the set of filtered syntax axioms.
            insert(*p);
//std::cout << "\\/ ";
        }
    }
}

// Filter syntax axioms whose constants are all in exp.
Syntaxioms Syntaxioms::filterbyexp(Expression const & exp) const
{
    Syntaxioms filtered;
//std::cout << "Filtering syntax axioms against: " << exp;
    // Constants in exp
    std::set<strview> exp2;
    std::remove_copy_if(exp.begin() + 1, exp.end(), end_inserter(exp2), isvar);
    // Scan and add syntax axioms with no constant symbols.
    filtered.addfromvec(exp2, keyis(""));
    // Scan and add syntax axioms whose key appears in exp.
    FOR (strview str, exp2)
    {
//std::cout << str.c_str << ' ';
        filtered.addfromvec(exp2, keyis(str.c_str));
    }
//std::cout << std::endl;
    return filtered;
}

// Return the revPolish notation of exp. Return the empty proof iff not okay.
Proofsteps Syntaxioms::rPolish
    (Expression const & exp, Disjvars const & disjvars) const
{
    if (exp.empty()) return Proofsteps();

    Syntaxioms const & filtered(filterbyexp(exp));
    Subexprecords recs;
    Substframe::Subexpends const & ends
        (rPolishmap(exp[0], exp.begin() + 1, exp.end(), exp, disjvars,
                    filtered, recs));

    Substframe::Subexpends::const_iterator const iter(ends.find(exp.end()));
    return iter == ends.end() ? Proofsteps() : iter->second;
}

// Find the revPolish notation and its tree of exp. Return true iff okay.
bool Syntaxioms::rPolish
    (Expression const & exp, Disjvars const & disjvars,
     Proofsteps & proofsteps, Prooftree & tree) const
{
    proofsteps = rPolish(exp, disjvars);
    if (unexpected(proofsteps.empty(), "rPolish error", exp))
        return false;
    tree = prooftree(proofsteps);
    if (unexpected(tree.empty(), "rPolish tree error", proofsteps))
        return false;
    return true;
}

// Find the revPolish notation of the whole assertion.  Return true iff okay.
bool Syntaxioms::rPolish
    (Assertion & ass, struct Typecodes const & typecodes) const
{
    Expression exp(ass.expression);
    if (exp.empty())
        return false;
    exp[0] = typecodes.normalize(exp[0]);
    if (!rPolish(exp, ass.disjvars, ass.exprPolish, ass.exptree))
        return false;
    // Preallocate for efficiency.
    ass.hypsrPolish.resize(ass.hypcount());
    ass.hypstree.resize(ass.hypcount());
    for (Hypsize i(0); i < ass.hypcount(); ++i)
    {
        Hypiter const iter(ass.hypiters[i]);
        exp = iter->second.first;
        if (exp.empty())
            return false;
        if (iter->second.second)
        {
            // Floating hypothesis
            ass.hypsrPolish[i] = Proofsteps(1, &*iter);
            ass.hypstree[i].assign(1, Prooftreehyps());
        }
        else
        {
            // Essential hypothesis
            exp[0] = typecodes.normalize(exp[0]);
            if (!rPolish(exp,ass.disjvars,ass.hypsrPolish[i],ass.hypstree[i]))
                return false;
        }
    }

    return true;
}

// Find the revPolish notation of a set of assertions.  Return true iff okay.
bool Syntaxioms::rPolish
    (Assertions & assertions, struct Typecodes const & typecodes) const
{
    std::cout << "Parsing syntax trees";
    Progress progress;
    Timer timer;
    Assertions::size_type count(0), all(assertions.size());
    FOR (Assertions::reference ass, assertions)
    {
        if (!rPolish(ass.second, typecodes))
        {
            printass(ass, count);
            std::cerr << "\nSyntax error!" << std::endl;
            return false;
        }
        progress << ++count/static_cast<double>(all);
    }

    std::cout << "done in " << timer << 's' << std::endl;
    return true;
}
