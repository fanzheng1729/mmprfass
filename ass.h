#ifndef ASS_H_INCLUDED
#define ASS_H_INCLUDED

#include <algorithm>
#include "proof/analyze.h"
#include "util.h"
#include "util/for.h"

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
    // Map: variable used in statement -> is used in the expression
    Varsused varsused;
    // #s key hypotheses: essential ones containing all free variables
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
        PROPOSITIONAL = 4
    };
    unsigned type;
// Functions:
    // # of hypotheses
    Hypsize hypcount() const {return hypiters.size();}
    // Return index the hypothesis matching the expression.
    // If there is no match, return # hypotheses.
    Hypsize matchhyp(Expression const & exp) const;
    Hypsize matchhyp(Proofsteps const & rPolish, strview type) const;
    // Check if the expression matches a hypothesis.
    bool istrivial(Expression const & exp) const
    { return matchhyp(exp) < hypcount(); }
    bool istrivial(Proofsteps const & rPolish, strview typecode) const
    { return matchhyp(rPolish, typecode) < hypcount(); }
    // length of the rev Polish notation of all hypotheses combined
    Expression::size_type hypslen() const
    {
        Expression::size_type len(0);
        for (Hypsize i(0); i < hypcount(); ++i)
            len += hypsrPolish[i].size();
        return len;
    }
    // # of variables
    Varsused::size_type varcount() const { return varsused.size(); }
    // # of variables in the expression
    Varsused::size_type expvarcount() const
    {
        Varsused::size_type n(0);
        FOR (Varsused::const_reference var, varsused)
            n += var.second.back();
        return n;
    }
    // Check if the expression is an equality
    bool isexpeq(Equalities const & equalities) const
    { return equalities.count(exprPolish.back()); }
    // If rPolish of the expression matches a given pattern,
    // return the proof string number 0.
    // Otherwise return NULL.
    const char * match(const int pattern[]) const;
};

// Find the equivalence relations and their justifications among syntax axioms.
Equalities equalities(Assertions const & assertions);

#endif // ASS_H_INCLUDED
