#include <algorithm>
#include <functional>
#include "ass.h"
#include "disjvars.h"
#include "io.h"
#include "parse.h"
#include "syntaxiom.h"
#include "util.h"
#include "util/find.h"
#include "util/for.h"

// Stack of substitution frames
typedef std::vector<Substframe> Substack;

// Find the substitution frame for a variable and return its address.
// Return NULL if not found, or the frame is invalid.
static Substframe const * findframe(strview var, Substack const & stack)
{
    Substack::const_iterator const iter(util::find(stack, var));
    return iter != stack.end() && *iter ? &*iter : NULL;
}

// Move matchbegin to next variable, or next constant not matching exp.
// Move expbegin by a corresponding offset. Return if a new variable is found.
static bool nextvar(Expiter & expbegin, Expiter expend,
                    Expiter & matchbegin, Expiter matchend)
{
    if (matchend <= matchbegin)
        return false;
    // Find first variable.
    Expiter matchend2(std::find_if(matchbegin, matchend, isvar));
    // Find first mismatch.
    matchend2 = util::mismatch(matchbegin, matchend2, expbegin, expend).first;
    // Move the iterators.
    expbegin += matchend2 - matchbegin;
    matchbegin = matchend2;
    // Return true if a new variable is found and 2 expressions matched there.
    return matchend2 != matchend && *matchend2;
}

// Move expbegin past end of substitution, matchbegin to the new variable.
// Return true if a new variable is found.
static bool nextnewvar(Expiter & expbegin, Expiter expend,
                       Expiter & matchbegin, Expiter matchend,
                       Substack const & stack)
{
    while (nextvar(expbegin, expend, matchbegin, matchend))
    {
//std::cout << "Found a variable " << *matchbegin << ". Is it new?\n";
        Substframe const * pframe(findframe(*matchbegin, stack));
        if (!pframe)
            return true; // *matchbegin is a new variable.

        Expiter const begin(pframe->begin), end(pframe->itersub->first);
//std::cout << *matchbegin << "=?=" << Expression(begin, end);
        if (expend-expbegin < end-begin || !std::equal(begin, end, expbegin))
            return false; // New substitution does not match old one.
        // Yes! Move the iterators.
        expbegin += end - begin;
        ++matchbegin;
    }

    return false;
}

// Check if two substitution frames satisfy the disjoint variable hypothesis.
static bool checkdisjvars
    (Substframe const * pframe1, Substframe const * pframe2,
     Disjvars const & disjvars)
{
    bool const okay(checkdisjvars(pframe1->begin, pframe1->itersub->first,
                                  pframe2->begin, pframe2->itersub->first,
                                  disjvars));
    if (!okay)
        std::cerr << "in substitutions for " << pframe1->var
                  << " and " << pframe2->var << std::endl;
    return okay;
}

// Check if substitutions for disjoint vars2 satisfy disjoint vars1.
static bool checkdisjvars
    (Disjvars const & disjvars, Disjvars const & disjvars2,
     Substack const & stack)
{
    FOR (Disjvars::const_reference var2, disjvars2)
    {
        Substframe const * pframe1(findframe(var2.first, stack));
        if (!pframe1)
            continue; // variable not substituted yet
        if (!*pframe1)
            return false; // bad frame
        Substframe const * pframe2(findframe(var2.second, stack));
        if (!pframe2)
            continue; // variable not substituted yet
        if (!*pframe2)
            return false; // bad frame
        // Now check the two substitutions for disjoint variable constraints.
        if (!checkdisjvars(pframe1, pframe2, disjvars))
            return false;
    }

    return true;
}

// Extract proof pointers from the stack.
// Return a single NULL if not okay.
static pProofs pproofsfromstack(Assertion const & ass, Substack const & stack)
{
    // Pointers to the proofs to be included
    pProofs pproofs;
    pproofs.reserve(ass.hypcount()); // Preallocate for efficiency

    FOR (Hypiter iter, ass.hypotheses)
    {
        Hypothesis const & hyp(iter->second);
        if (unexpected(!hyp.second, "essential hypothesis", iter->first))
            return pProofs(1);
        // Hypothesis is floating. Find the frame for the substitution.
        strview var(hyp.first[1]);
        Substframe const * pframe(findframe(var, stack));
        if (unexpected(!pframe, "variable", var))
            return pProofs(1);
        // *iterframe is the right frame. Record the proof pointer.
        pproofs.push_back(&pframe->itersub->second);
    }

    return pproofs;
}

// If substitution in not empty, create a frame for them and return true.
static bool createsubstitutionframe
    (Substack & stack, strview var, Expiter begin,
     Substframe::Subexpends const & ends)
{
    if (ends.empty())
        return false;

    Substframe frame = {var, begin, ends, ends.begin()};
    stack.push_back(frame);
    return true;
}

// Return true iff a new variable frame is created.
static bool nextsubframe
    (Expiter expbegin, Expiter expend,
     Expression const & exp, Disjvars const & disjvars,
     Syntaxioms const & syntaxioms, Assiter iter, Subexprecords & recs,
     // Track the substitutions for variables.
     Substack & stack, Substframe::Subexpends & result);

// Return possible substitutions from begin to end of the given type.
Substframe::Subexpends const & rPolishmap
    (strview type, Expiter expbegin, Expiter expend,
     Expression const & exp, Disjvars const & disjvars,
     struct Syntaxioms const & syntaxioms, Subexprecords & recs)
{
    // Reserve space for substitutions from begin to end, inclusive.
    recs[type].resize(exp.size() + 1);

    // Check if expbegin has been seen before.
    std::pair<Substframe::Subexpends, bool> & iter
        (recs[type][expbegin - exp.begin()]);
    if (iter.second)
        // Yes. Return the recorded result.
        return iter.first;

    iter.second = true;
    Substframe::Subexpends & result(iter.first);

    // Check if exp contains a type declaration.
    if (expbegin != expend && expbegin->phyp != NULL &&
        expbegin->typecode() == type)
        result[expbegin + 1].assign(1, expbegin->phyp);

    // Match syntax axioms.
    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
        Assiter assiter(syntaxiom.second.assiter);
        Expression const & saexp(assiter->second.expression);
        if (saexp.empty() || saexp[0] != type)
            continue; // Type mismatch
//std::cout << "Matching " << syntaxiom.first
//          << " against " << Expression(expbegin, expend);
        Substack stack;
        do
        {
            if (nextsubframe(expbegin, expend, exp, disjvars,
                             syntaxioms, assiter, recs, stack, result))
                continue; // A new variable and possible substitutions found.

            if (stack.empty())
                continue; // No variable matched. Try next syntax axiom.

            // Try the next substitution.
            while (++stack.back().itersub == stack.back().ends.end())
            {
//std::cout << "Popping frame for " << stack.back().var << std::endl;
                stack.pop_back();
                if (stack.empty()) break;
            }
        } while(!stack.empty());
    }

    return result;
}

// Return true iff a new frame is created.
static bool nextsubframe
    (Expiter expbegin, Expiter expend,
     Expression const & exp, Disjvars const & disjvars,
     Syntaxioms const & syntaxioms, Assiter iter, Subexprecords & recs,
     // Track the substitutions for variables.
     Substack & stack, Substframe::Subexpends & result)
{
    // The syntax axiom to be matched
    Assertion const & ass(iter->second);
    // The pattern to be matched
    Expression const & pattern(ass.expression);
    // Iterators to exp and the pattern
    Expiter iter1(expbegin), iter2(pattern.begin() + 1);

    if (!stack.empty()) // Some variables have been substituted.
    {
        // Move iter1 past end of the last substitution.
        iter1 = stack.back().itersub->first;
        // Move iter2 past the variable.
        iter2 = util::find(pattern, stack.back()) + 1;
    }

    // Find a new variable.
//std::cout << "Matching " << Expression(iter1, expend);
//std::cout << "Against " << Expression(iter2, pattern.end());
    if (!nextnewvar(iter1, expend, iter2, pattern.end(), stack))
    {
//std::cout << "No more variable found" << std::endl;
        if (iter2 == pattern.end() &&
            checkdisjvars(disjvars, ass.disjvars, stack))
        {
            // exp contains the pattern.
            pProofs const & hyps(pproofsfromstack(ass, stack));
            if (!hyps.empty() && !hyps[0])
                return false;
            writeproof(result[iter1], &*iter, hyps);
        }
        return false;
    }

    strview nextvartype(iter2->typecode());
//std::cout << "New var " << *iter2 << ": " << nextvartype << std::endl;
//std::cout << "New substitutions are in " << Expression(iter1, expend);

    // Push valid substitutions for *iter2 onto the stack.
    Substframe::Subexpends const & substitutions
        (rPolishmap(nextvartype, iter1, expend, exp, disjvars, syntaxioms, recs));
    return createsubstitutionframe(stack, *iter2, iter1, substitutions);
}
