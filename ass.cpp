#include "ass.h"
#include "util/filter.h"
#include "util/for.h"

// Return index the hypothesis matching the expression.
// If there is no match, return # hypotheses.
Hypsize Assertion::matchhyp(Expression const & exp) const
{
    Hypsize i(0);
    for ( ; i < hypcount(); ++i)
        if (hypiters[i]->second.first == exp)
            return i;

    return i;
}
Hypsize Assertion::matchhyp(Proofsteps const & rPolish, strview typecode) const
{
    Hypsize i(0);
    for ( ; i < hypcount(); ++i)
    {
        Hypothesis const & hyp(hypiters[i]->second);
        if (!hyp.first.empty() && hyp.first[0] == typecode &&
            hypsrPolish[i] == rPolish)
            return i;
    }

    return i;
}

// If rPolish of the expression matches a given pattern,
// return the proof string number 0.
// Otherwise return NULL.
const char * Assertion::match(const int pattern[]) const
{
    // End of pattern
    const int * end(pattern);
    while (*end != -1) end++;
    // Size of the rPolish chained together
    Proofsize proofsize(0);
    for (Hypsize i(0); i < hypcount(); ++i)
    {
        if (hypiters[i]->second.second)
            continue; // Skip floating hypotheses.

        proofsize += hypsrPolish[i].size();
        if (pattern + proofsize > end || pattern[proofsize - 1] != 0 ||
            hypsrPolish[i].back() != exprPolish.back())
            return NULL;
    }
    proofsize += exprPolish.size();
    if (pattern + proofsize != end)
        return NULL;
    // rPolish of the hypotheses and the expression chained together
    Proofsteps proof;
    proof.reserve(proofsize);
    for (Hypsize i(0); i < hypcount(); ++i)
        if (!hypiters[i]->second.second)
            proof += hypsrPolish[i];
    proof += exprPolish;
    // # arguments.
    unsigned const argcount(*std::max_element(pattern, end));
    if (varcount() != argcount)
        return NULL;
    // Substitution vector
    Proofsteps substitutions(argcount + 1);
    // Check the pattern.
    for (const int * cur(pattern); cur < end; ++cur)
    {
        if (substitutions[*cur].type == Proofstep::NONE)
            substitutions[*cur] = proof[cur - pattern];
        else if (substitutions[*cur] != proof[cur - pattern])
            return NULL;
    }

    return exprPolish.back();
}

//#include <iostream>
// Find the equivalence relations and their justifications among syntax axioms.
Equalities equalities(Assertions const & assertions)
{
    static const int patterns[][10] =
    {
        {1, 1, 0, -1}, // Equivalence
        {1, 2, 0, 2, 1, 0, -1}, // Reflexivity
        {1, 2, 0, 2, 3, 0, 1, 3, 0, -1} // Transitivity
    };

    Equalities result;
    FOR (Assertions::const_reference ass, assertions)
    {
//std::cout << ass.first.c_str;
        for (int i(0); i < 3; ++i)
        {
            if (const char * label = ass.second.match(patterns[i]))
            {
#if __cplusplus < 201103L
                result[label].resize(3);
#endif // __cplusplus < 201103L
                result[label][i] = ass.first.c_str;
            }
        }
    }

    for (Equalities::iterator iter(result.begin()); iter != result.end(); )
    {
        if (!util::filter(iter->second)(static_cast<const char *>(NULL)))
            ++iter;
        else
            result.erase(iter++);
    }

    return result;
}
