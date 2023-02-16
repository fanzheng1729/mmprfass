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
        std::vector<Hypiter>::const_iterator const
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
Disjvars Scopes::disjvars(std::set<strview> const & varsused) const
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
                    result.insert(std::make_pair(*diter, *diter2));
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
    std::set<strview> varsused;
    FOR (Symbol3 symbol, exp)
        if (symbol.phyp)
        {
            varsused.insert(symbol);
            ass.varsused[symbol] = Bvector(1, true);
//std::cout << symbol << " 1\t";
        }

    for (const_reverse_iterator iter(rbegin()); iter != rend(); ++iter)
    {
        std::vector<Hypiter> const & hypvec(iter->activehyp);
        for (std::vector<Hypiter>::const_reverse_iterator iter2
            (hypvec.rbegin()); iter2 != hypvec.rend(); ++iter2)
        {
            Hypiter const iterhyp(*iter2);
//std::cout << "Checking hypothesis " << iterhyp->first << std::endl;
            Hypothesis const & hyp(iterhyp->second);
            Expression const & hypexp(hyp.first);
            if (hyp.second && varsused.count(hypexp[1]) > 0)
            {
//std::cout << "Mandatory floating Hypothesis: " << hypexp;
                ass.hypotheses.push_front(iterhyp);
                ass.varsused.insert(std::make_pair(hypexp[1],
                                                   Bvector(1, false)));
//std::cout << hypexp[1] << " 0\t";
            }
            else if (!hyp.second)
            {
//std::cout << "Essential hypothesis: " << hypexp;
                ass.hypotheses.push_front(iterhyp);
                // Add variables used in hypotheses
                FOR (Symbol3 symbol, hypexp)
                    if (symbol.phyp)
                        varsused.insert(symbol);
//std::cout << varsused.size() << " variables used so far" << std::endl;
            }
        }
    }
//std::cin.get();
    ass.disjvars = disjvars(varsused);
    // Find key hypotheses.
    if (ass.varcount() == ass.expvarcount())
        return;
    for (Hypsize i(0); i < ass.hypcount(); ++i)
    {
        Hypiter const iter(ass.hypotheses[i]);
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
