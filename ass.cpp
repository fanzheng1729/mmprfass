#include "ass.h"
#include "util/filter.h"
#include "util/for.h"

// Remove unnecessary variables.
Bvector & Assertion::trimvars
    (Bvector & hypstotrim, Proofsteps const & conclusion) const
{
//std::cout << conclusion;
    hypstotrim.resize(hypcount());
    hypstotrim.flip();
    for (Hypsize i(0); i < hypcount(); ++i)
    {
        Hypothesis const & hyp(hypiters[i]->second);
        if (!hyp.second)
            continue; // Skip essential hypotheses.
        // Appearance of the variable in hypotheses
        Bvector const & usage(varsused.at(hyp.first[1]));
//std::cout << "use of " << hyp.first[1] << ' ';
        Hypsize j(hypcount() - 1);
        for ( ; j != Hypsize(-1); --j)
            if (hypstotrim[j] && usage[j])
                break;
//std::cout << j << ' ';
        hypstotrim[i] = (j != Hypsize(-1) ||
                        util::filter(conclusion)(hypiters[i]->first.c_str));
    }
    hypstotrim.flip();
//std::cout << hypstotrim;
    return hypstotrim;
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

// Set the hypotheses, trimming away specified ones.
void Assertion::sethyps(Assertion const & ass, Bvector const & hypstotrim)
{
    if (hypstotrim.empty())
        return;
    hypiters.clear(), hypsrPolish.clear(), hypstree.clear();
    varsused.clear();
    for (Hypsize i(0); i < ass.hypcount(); ++i)
    {
        if (i < hypstotrim.size() && hypstotrim[i])
            continue;
        // *iter is used.
        Hypiter const iter(ass.hypiters[i]);
        hypiters.push_back(iter);
        if (iter->second.second)
        {
            // Floating hypothesis of used variable
            Symbol3 const var(iter->second.first[1]);
            Bvector & usage(varsused[var]);
            Bvector const & assusage(ass.varsused.at(var));
            for (Hypsize j(0); j < ass.hypcount(); ++j)
                if (!hypstotrim[j])
                    usage.push_back(assusage[j]);
        }
        hypsrPolish.push_back(ass.hypsrPolish[i]);
        hypstree.push_back(ass.hypstree[i]);
    }
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
