#ifndef ASS_H_INCLUDED
#define ASS_H_INCLUDED

#include "proof/analyze.h"

// An axiom or a theorem.
struct Assertion
{
// Essential properties:
    // Hypotheses of this axiom or theorem.
    Hypiters hypiters;
    Disjvars disjvars;
    // Statement of axiom or theorem.
    Expression expression;
    // # of axiom or theorem as ordered in the input file, starting from 1.
    Assertions::size_type number;
    operator Assertions::size_type() const { return number; }
// Derived properties:
    // # free variables
    Hypsize nfreevar;
    // Map: variable used in statement -> (is used in hyp i, is used in exp)
    Varsused varsused;
    // Index of key hypotheses: essential ones containing all free variables
    std::vector<Hypsize> keyhyps;
    // revPolish notation of hypotheses.
    std::vector<Proofsteps> hypsrPolish;
    // revPolish of expression, proof steps.
    Proofsteps exprPolish, proofsteps;
    // Proof tree of revPolish of expression.
    Prooftree exptree;
    // Proof tree of revPolish of hypotheses.
    std::vector<Prooftree> hypstree;
    // Type (propositional, predicate, etc).
    enum Type
    {
        AXIOM = 1,
        TRIVIAL = 2,
        DUPLICATE = 4,
        USELESS = TRIVIAL | DUPLICATE,
        PROPOSITIONAL = 8
    };
    unsigned type;
// Functions:
    // # of hypotheses
    Hypsize hypcount() const {return hypiters.size();}
    // Return index the hypothesis matching the expression.
    // If there is no match, return # hypotheses.
    Hypsize matchhyp(Expression const & exp) const
    {
        Hypsize i(0);
        for ( ; i < hypcount(); ++i)
            if (hypiters[i]->second.first == exp)
                return i;
        return i;
    }
    Hypsize matchhyp(Proofsteps const & rPolish, strview typecode) const
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
    // Check if the expression matches a hypothesis.
    bool istrivial(Expression const & exp) const
    { return matchhyp(exp) < hypcount(); }
    bool istrivial(Proofsteps const & rPolish, strview typecode) const
    { return matchhyp(rPolish, typecode) < hypcount(); }
    // Remove unnecessary variables.
    Bvector trimvars
        (Bvector const & extrahyps, Proofsteps const & conclusion) const
    {
        Bvector result(extrahyps);
        return trimvars(result, conclusion);
    }
    Bvector & trimvars
        (Bvector & extrahyps, Proofsteps const & conclusion) const;
    // Length of the rev Polish notation of all necessary hypotheses combined
    Expression::size_type hypslen(Bvector const & extrahyps = Bvector()) const
    {
        Expression::size_type len(0);
        for (Hypsize i(0); i < hypcount(); ++i)
            len += !(i < extrahyps.size() && extrahyps[i])
                * hypsrPolish[i].size();
        return len;
    }
    // Label of all necessary hypotheses combined, separated by delim
    std::string hypslabel
        (Bvector const & extrahyps = Bvector(), char delim = '+') const
    {
        std::string result;
        for (Hypsize i(0); i < hypcount(); ++i)
            if (!(i < extrahyps.size() && extrahyps[i]))
                (result += delim) += hypiters[i]->first;
        return result;
    }
    // # of variables
    Varsused::size_type varcount() const { return varsused.size(); }
    // # of essential hypotheses
    Hypsize esshypcount() const { return hypcount() - varcount(); }
    // Check if the expression is an equality
    bool isexpeq(Equalities const & equalities) const
    { return equalities.count(exprPolish.back()); }
    // If rPolish of the expression matches a given pattern,
    // return the proof string number 0.
    // Otherwise return NULL.
    const char * match(const int pattern[]) const;
// Modifying functions
    // Set the hypotheses, throwing away extra ones.
    void sethyps(Assertion const & ass, Bvector const & extrahyps = Bvector());
};

// Find the equivalence relations and their justifications among syntax axioms.
Equalities equalities(Assertions const & assertions);

#endif // ASS_H_INCLUDED
