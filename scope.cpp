#include <iostream>
#include "ass.h"
#include "getproof.h"
#include "util/filter.h"
#include "util/find.h"
#include "util/for.h"
#include "msg.h"
#include "scope.h"

// Determine if there is only one scope. If not then print error.
bool Scopes::isouter(const char * msg) const
{
    if (size() != 1 && msg)
        std::cerr << msg << std::endl;

    return size() == 1;
}

// Return true if nonempty. Otherwise return false and print error.
bool Scopes::pop_back()
{
    if (size() > 1)
    {
        std::vector<Scope>::pop_back();
        return true;
    }
    std::cerr << extraendscope << std::endl;
    return false;
}

// Find active floating hypothesis corresponding to variable.
// Return its name or NULL if there isn't one.
Hypptr Scopes::getfloatinghyp(strview var) const
{
    FOR (const_reference scope, *this)
    {
//std::cout << "scope " << &scope - data() << " has "
//          << scope.floatinghyp.size() << " variables\n";
        std::map<strview, Hypiter>::const_iterator const loc
            (scope.floatinghyp.find(var));
        if (loc != scope.floatinghyp.end())
            return &*loc->second;
//std::cout << var << " not found" << std::endl;
    }
    return NULL;
}

// Determine if a string is an active variable.
bool Scopes::isactivevariable(strview var) const
{
    FOR (const_reference scope, *this)
    {
        if (scope.activevariables.count(var) > 0)
            return true;
    }
    return false;
}

// Determine if a string is the label of an active hypothesis.
// If so, return the pointer to the hypothesis. Otherwise return NULL.
Hypptr Scopes::activehypptr(strview label) const
{
    FOR (const_reference scope, *this)
    {
        Hypiters::const_iterator const
            iterhypvec(util::find(scope.activehyp, label));
        if (iterhypvec != scope.activehyp.end())
            return &**iterhypvec;
    }
    return NULL;
}

// Determine if a floating hypothesis on a string can be added.
// Return 0 if Okay. Otherwise return error code.
int Scopes::erraddfloatinghyp(strview var) const
{
    return isactivevariable(var) ? !!getfloatinghyp(var) * VARDEFINED :
        VARNOTACTIVE;
}

// Determine if there is an active disjoint variable restriction on
// two different variables.
bool Scopes::isdvr(strview var1, strview var2) const
{
    if (var1 == var2)
        return false;

    FOR (const_reference scope, *this)
        FOR (Symbol2s const & vars, scope.disjvars)
            if (vars.count(var1) && vars.count(var2))
                return true;

    return false;
}

// Determine mandatory disjoint variable restrictions.
Disjvars Scopes::disjvars(Symbol2s const & varsused) const
{
    Disjvars result;

    FOR (const_reference scope, *this)
        FOR (Symbol2s const & vars, scope.disjvars)
        {
            Symbol2s dset(vars & varsused);
            for (Symbol2s::iterator diter
                 (dset.begin()); diter != dset.end(); ++diter)
            {
                Symbol2s::iterator diter2(diter);
                for (++diter2; diter2 != dset.end(); ++diter2)
                    result.insert(Disjvars::value_type(*diter, *diter2));
            }
        }

    return result;
}

// Complete an Assertion from its Expression. That is, determine the
// mandatory hypotheses and disjoint variable restrictions and the #.
void Scopes::completeass(struct Assertion & ass) const
{
    Expression const & exp(ass.expression);

    // Determine variables used and find mandatory hypotheses
    Symbol2s varsused;
    FOR (Symbol3 var, exp)
        if (var)
        {
            varsused.insert(var);
            ass.varsused[var].assign(1, true);
//std::cout << var << " ";
        }

    for (const_reverse_iterator iter(rbegin()); iter != rend(); ++iter)
    {
        Hypiters const & hypvec(iter->activehyp);
        for (Hypiters::const_reverse_iterator iter2
            (hypvec.rbegin()); iter2 != hypvec.rend(); ++iter2)
        {
            Hypiter const iterhyp(*iter2);
//std::cout << "Checking hypothesis " << iterhyp->first << std::endl;
            Hypothesis const & hyp(iterhyp->second);
            Expression const & hypexp(hyp.first);
            if (hyp.second && varsused.count(hypexp[1]) > 0)
            {
//std::cout << "Mandatory floating Hypothesis: " << hypexp;
                ass.hypiters.push_back(iterhyp);
                Varsused::value_type value(hypexp[1], Bvector(1, false));
                ass.varsused.insert(value);
//std::cout << hypexp[1] << " 0\t";
            }
            else if (!hyp.second)
            {
//std::cout << "Essential hypothesis: " << hypexp;
                ass.hypiters.push_back(iterhyp);
                // Add variables used in hypotheses
                FOR (Symbol3 var, hypexp)
                    if (var)
                    {
                        varsused.insert(var);
//std::cout << var << " ";
                    }
            }
        }
    }

    std::reverse(ass.hypiters.begin(), ass.hypiters.end());

    ass.disjvars = disjvars(varsused);
    // Find key hypotheses.
    if (ass.varcount() == ass.expvarcount())
        return;
    for (Hypsize i(0); i < ass.hypcount(); ++i)
    {
        Hypiter const iter(ass.hypiters[i]);
        if (iter->second.second)
            continue; // Skip floating hypotheses.
        // Expression of the hypothesis
        Expression const & hypexp(iter->second.first);
        // Check if the hypothesis is key hypothesis.
        bool iskey(true);
        FOR (Varsused::const_reference var, ass.varsused)
            if (!var.second.back() && !util::filter(hypexp)(var.first))
                iskey = false;
        // If it is, note it.
        if (iskey)
            ass.keyhyps.push_back(i);
    }
}
