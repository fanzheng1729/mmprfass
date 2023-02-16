#include "ass.h"
#include "util/find.h"

// Return iterator to the hypothesis matching the expression.
// If there is no match, return the end iterator.
Hypiteriter Assertion::matchhyp(Expression const & exp) const
{
    Hypiteriter iter(hypotheses.begin());
    for ( ; iter != hypotheses.end(); ++iter)
    {
        Hypothesis const & hyp((*iter)->second);
        if (hyp.second) // Skip floating hypothesis.
            continue;

        if (hyp.first == exp)
            return iter; // A hypothesis matched
    }

    return iter;
}
Hypiteriter Assertion::matchhyp
    (Proofsteps const & rPolish, strview typecode) const
{
    Hypiteriter iter(hypotheses.begin());
    for ( ; iter != hypotheses.end(); ++iter)
    {
        Hypothesis const & hyp((*iter)->second);
        if (hyp.second) // Skip floating hypothesis.
            continue;
//std::cout << "Matching " << rPolish << "Against " << hyp.first;
        if (!hyp.first.empty() && hyp.first[0] == typecode &&
            hypsrPolish[iter - hypotheses.begin()] == rPolish)
            return iter;
    }

    return iter;
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
        bool const isfloating(hypotheses[i]->second.second);
        proofsize += !isfloating * hypsrPolish[i].size();
        if (pattern + proofsize > end || pattern[proofsize - 1] != 0)
            return NULL;
        if (!isfloating && hypsrPolish[i].back() != exprPolish.back())
            return NULL;
    }
    proofsize += exprPolish.size();
    if (pattern + proofsize != end)
        return NULL;
    // rPolish of the hypotheses and the expression chained together
    Proofsteps proof;
    proof.reserve(proofsize);
    for (Hypsize i(0); i < hypcount(); ++i)
        if (!hypotheses[i]->second.second)
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
        for (int i(0); i < 3; ++i)
            if (const char * label = ass.second.match(patterns[i]))
            {
#if __cplusplus < 201103L
                result[label].resize(3);
#endif // __cplusplus < 201103L
                result[label][i] = ass.first.c_str;
            }

    for (Equalities::iterator iter(result.begin()); iter != result.end(); )
    {
        if (util::find(iter->second, (const char *)(0)) == iter->second + 3)
            ++iter;
        else
            result.erase(iter++);
    }

    return result;
}
