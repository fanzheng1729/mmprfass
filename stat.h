#ifndef STAT_H_INCLUDED
#define STAT_H_INCLUDED

#include <algorithm>
#include "ass.h"
#include "util/for.h"
#include "io.h"
#include "proof/step.h"
#include "syntaxiom.h"

// Check if all symbols in a revPolish notation are defined.
// If so, return the largest # of def/syntax axiom.
// If a symbol has no definition, its # is n. Otherwise return 0.
template<class T>
Assertions::size_type largestsymboldefnumber
    (Proofsteps const & proofsteps, T const & definitions,
     Syntaxioms const & syntaxioms, Assertions::size_type const n)
{
    Assertions::size_type result(1);
//std::cout << definitions << syntaxioms;
    FOR (Proofstep step, proofsteps)
    {
//std::cout << step << ':';
        if (step.type == Proofstep::HYP)
            continue; // variable
        Assertions::size_type number(0);
        typename T::key_type const label(step);
        typename T::const_iterator iterdf(definitions.find(label));
//std::cout << "sa";
        if (iterdf != definitions.end())
            number = iterdf->second.pdef ? iterdf->second : n;
        else
        {
//std::cout << "ud";
            Syntaxioms::const_iterator itersyn(syntaxioms.find(strview(step)));
            if (itersyn != syntaxioms.end())
                number = itersyn->second; // found in syntax axioms
            else
                return 0; // undefined symbol
        }
//std::cout << number << '\t';
        result = std::max(result, number ? number : 1);
    }

    return result;
}

// Check if all symbols in an assertion are defined.
// If so, return the largest # of def/syntax axiom.
// If a symbol has empty definition, return n. Otherwise return 0.
template<class T>
Assertions::size_type largestsymboldefnumber
    (Assertion const & ass, T const & definitions,
     Syntaxioms const & syntaxioms, Assertions::size_type const n = 0)
{
    Assertions::size_type result(0);

    result =
        largestsymboldefnumber(ass.exprPolish, definitions,
                               syntaxioms, n);
//std::cout << ass.expression << "has number " << result << std::endl;
    if (result == 0)
        return 0;
    // Check the hypotheses.
    FOR (Proofsteps const & steps, ass.hypsrPolish)
    {
        Assertions::size_type const result2 =
            largestsymboldefnumber(steps, definitions, syntaxioms, n);
        if (result2 == 0)
            return 0;
//std::cout << "hyp " << &steps - hypproofs.data() << ':' << result2 << '\t';
        result = std::max(result, result2);
//std::cout << "has number " << result << std::endl;
    }

    return result;
}

// Return the largest # of syntax axiom in a proof.
inline Assertions::size_type largestsymboldefnumber
    (Proofsteps const & proofsteps, Syntaxioms const & syntaxioms)
{
    Assertions::size_type result(0);

    FOR (Proofstep step, proofsteps)
    {
//std::cout << step << ':';
        if (step.type != Proofstep::ASS)
            continue;

        if (syntaxioms.count(strview(step)))
            result = std::max(result, step.pass->second.number);
    }

    return result;
}

// Check if an assertion is hard, i.e., uses a new syntax in its proof.
inline bool isasshard(Assertion const & ass, Syntaxioms const & syntaxioms)
{
    // Largest # of syntax axiom in the assertion
    Assertions::size_type const symbolnumber
        (largestsymboldefnumber(ass, Definitions(), syntaxioms));
    if (symbolnumber == 0)
        return false; // Undefined syntax
    // Largest # of syntax axiom in the proof
    Assertions::size_type const proofsymbolnumber
        (largestsymboldefnumber(ass.proofsteps, syntaxioms));
    if (proofsymbolnumber == 0)
        return false; // Undefined syntax
    return proofsymbolnumber > symbolnumber;
}

#endif // STAT_H_INCLUDED
