#include <algorithm>
#include <iterator>
#include "comment.h"
#include "def.h"
#include "disjvars.h"
#include "io.h"
#include "proof/analyze.h"
#include "syntaxiom.h"

static const char df[] = "df-";

// Find the revPolish notation of (LHS, RHS).
Definition::Definition(Assertions::const_reference rass) : pdef(NULL)
{
    strview label(rass.first);
    Assertion const & ass(rass.second);
    Proofsteps const & proofsteps(ass.exprPolish);
    Prooftree const & tree(prooftree(proofsteps));

    if (tree.empty())
    {
        std::cerr << ass.expression << "has bad syntax proof:\n" << proofsteps;
        if (proofsteps.empty())
            std::cerr << "Database::rPolish should be run first" << std::endl;
        return;
    }

    // Index of the syntax to be defined
    Proofsize const lhsend(tree.back()[0]);
    // Index of beginning of RHS
    Proofsize const defbegin(lhsend + 1);

    if (tree.back().size() != 2)
    {
        strview root(proofsteps.back());
        std::cerr << label << "\troot symbol " << root << " not binary\n";
        return;
    }
    // Check if LHS = (var, ..., var, syntax to be defined).
    if (tree[lhsend].size() != lhsend)
    {
        std::cerr << label << "\tbad LHS: " << proofsteps;
        return;
    }

    pdef = &rass;
    lhs.assign(proofsteps.begin(), proofsteps.begin() + defbegin); // Never empty
    rhs.assign(proofsteps.begin() + defbegin, proofsteps.end() - 1);
}

// Check the required disjoint variable hypotheses (rules 3 & 4).
bool Definition::checkdv() const
{
    if (!pdef) return false;

    Varsused const & vars(pdef->second.varsused);
    Disjvars const & disjvars(pdef->second.disjvars);
    for (Varsused::const_iterator iter(vars.begin()); iter!=vars.end(); ++iter)
    {
        Symbol2 var(iter->first, iter->first);
        Varsused::const_iterator iter2(iter);
        for (++iter2; iter2 != vars.end(); ++iter2)
        {
            Symbol2 var2(iter2->first, iter2->first);
            if (disjvars.count(std::make_pair(var, var2))
                != (isdummy(iter->first) || isdummy(iter2->first)))
            {
                static const char * const dummy[] = {" not", ""};
std::cout << var.c_str << dummy[isdummy(iter->first)] << " dummy, & "
          << var2.c_str << dummy[isdummy(iter2->first)] << " dummy, "
          << "violate disjoint variable hypothesis\n";
                return false;
            }
        }
    }

    return true;
}

// Check if all dummy variables are bound (not fully implemented).
bool Definition::checkdummyvar(struct Typecodes const & typecodes) const
{
    FOR (Varsused::const_reference var, pdef->second.varsused)
        if (isdummy(var.first))
        {
            Typecodes::const_iterator iter2
                (typecodes.find(var.first.typecode()));
            if (iter2 == typecodes.end() || !iter2->second.second)
                return false;
        }

    return true;
}

static void printrule(strview label, int rule)
{
    static const char * const rules[] =
    {
        "", "has essential hypotheses", "root symbol not equality",
        "definition does not parse", "definition is circular",
        "bad disjoint variables", "has dummy non-set variables",
        "not propositional"
    };
    std::cout << label << '\t' << rules[rule] << std::endl;
}

// Check if an assertion is a definition. Return 0 if OK.
// Otherwise return error code.
Definition::Definition
    (Assertions::const_reference rass, struct Typecodes const & typecodes,
     Equalities const & equalities)
{
    int rule(0);
    Assertion const & ass(rass.second);
    if (ass.hypcount() > ass.varcount())
        printrule(rass.first, rule = 1);
    if (!ass.isexpeq(equalities))
        printrule(rass.first, rule = 2);
    if (rule)
    {
        pdef = NULL;
        return;
    }
    *this = Definition(rass);
    if (!pdef)
    {
        printrule(rass.first, rule = 3);
        return;
    }
    if (iscircular())
        printrule(rass.first, rule = 4);
    if (!checkdv())
        printrule(rass.first, rule = 5);
    if (!checkdummyvar(typecodes))
        printrule(rass.first, rule = 6);
    if (rule)
        pdef = NULL;
}

Definitions::Definitions
    (Assertions const & assertions, struct Commentinfo const & commentinfo,
     Equalities const & equalities)
{
    Typecodes const & typecodes(commentinfo.typecodes);
    Ctordefns const & ctordefns(commentinfo.ctordefns);
// Add definitions.
    FOR (Assertions::const_reference ass, assertions)
    {
        if (ass.first.remove_prefix(df))
            adddef(ass, typecodes, equalities);
    }
    // Adjust definitions.
    FOR (Ctordefns::const_reference ctor, ctordefns)
    {
        // syntax sa or definition df for sa
        key_type sa(ctor.first.c_str()), df(ctor.second.c_str());
        if (*df == 0) // Skip primitive syntax.
            continue;
        // Find assertion.
        Assiter newiter(assertions.find(df));
        if (unexpected(newiter == assertions.end(), "syntax definition", df))
            continue;
        // Adjust definition.
        std::cout << sa << "\tdefinition changed to " << df << std::endl;
        adddef(*newiter, typecodes, equalities);
    }
}

// Add a definition. Return true iff okay.
bool Definitions::adddef
    (Assertions::const_reference rass, struct Typecodes const & typecodes,
     Equalities const & equalities)
{
    Definition def(rass, typecodes, equalities);
    if (!def.pdef)
        return false;
    // Check for redefinition.
    key_type syntaxiom(def.pdef->second.exprPolish[def.lhs.size() - 1]);
    Definition & rdef((*this)[syntaxiom]);
    if (unexpected(rdef.pdef, "redefinition of", syntaxiom))
        return false;
    // Add definition.
    rdef = def;
    return true;
}
