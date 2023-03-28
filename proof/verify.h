#ifndef VERIFY_H_INCLUDED
#define VERIFY_H_INCLUDED

#include <algorithm>
#include "printer.h"
#include "../util/for.h"

// Extract proof steps from a compressed proof.
Proofsteps compressedproofsteps
    (Proofsteps const & labels, Proofnumbers const & proofnumbers);
// Extract proof steps from a regular proof.
Proofsteps regularproofsteps
    (Proof const & proof,
     Hypotheses const & hypotheses, Assertions const & assertions);

// Check if there are enough items on the stack for hypothesis verification.
bool enoughitemonstack
    (std::size_t hypcount, std::size_t stacksize, strview label);

void printunificationfailure
    (strview thlabel, strview reflabel, Hypothesis const & hyp,
     Expression const & dest, Expression const & stackitem);

// Append a subexpression to an expression.
template<class Iter>
Expression & operator+=(Expression & exp, std::pair<Iter, Iter> subexp)
{
    exp.insert(exp.end(), subexp.first, subexp.second);
    return exp;
}

// Preallocate space for substitutions.
template<class SUB>
void prealloc(std::vector<SUB> & substitutions, Varsused const & vars)
{
    if (vars.empty()) return;

    Symbol2::ID maxid(std::max_element(vars.begin(), vars.end())->first);

    substitutions.assign(maxid + 1, SUB());
}

// Must specialize in order to use the following.
template<class TOK, class SUB>
void makesubstitution(std::vector<TOK> const & src, std::vector<TOK> & dest,
                      std::vector<SUB> const & substitutions,
                      Symbol2::ID (TOK::*p)() const = Symbol2::operator ID)
{
    if (substitutions.empty())
        return dest.assign(src.begin(), src.end());

    FOR (TOK symbol, src)
        if (Symbol2::ID id = (symbol.*p)())
            dest += substitutions[id];
        else
            dest.push_back(symbol);
}

// Find the substitution. Increase the size of the stack by 1.
// Return index of the base of the substitution in the stack.
// Return the size of the stack if not okay.
template<class HYPS, class EXP, class SUB>
typename std::vector<EXP>::size_type findsubstitutions
    (strview label, strview reflabel, HYPS const & hypotheses,
     std::vector<EXP> & stack, std::vector<SUB> & substitutions)
{
    typename HYPS::size_type const hypcount(hypotheses.size());
    if (!enoughitemonstack(hypcount, stack.size(), label))
        return stack.size();

    // Space for new statement onto stack
    stack.resize(stack.size() + 1);

    typename HYPS::size_type const base(stack.size() - 1 - hypcount);

    // Determine substitutions and check that we can unify
    for (typename HYPS::size_type i(0); i < hypcount; ++i)
    {
        Hypothesis const & hypothesis(hypotheses[i]->second);
        if (hypothesis.second)
        {
            // Floating hypothesis of the referenced assertion
            if (hypothesis.first[0] != stack[base + i][0])
            {
                printunificationfailure(label, reflabel, hypothesis,
                                        hypothesis.first, stack[base + i]);
                return stack.size();
            }
            Symbol3::ID id(hypothesis.first[1]);
            substitutions.resize(std::max(id + 1, substitutions.size()));
            substitutions[id] = SUB(&stack[base + i][1],
                                    &stack[base + i].back() + 1);
        }
        else
        {
            // Essential hypothesis
            Expression dest;
            makesubstitution(hypothesis.first, dest, substitutions);
            if (dest != stack[base + i])
            {
                printunificationfailure(label, reflabel, hypothesis,
                                        dest, stack[base + i]);
                return stack.size();
            }
        }
    }

    return base;
}

// Subroutine for proof verification. Verify proof steps.
Expression verifyproofsteps(Proofsteps const & steps,
                            Printer & printer, Assptr pthm = 0);
inline Expression verifyproofsteps(Proofsteps const & steps, Assptr pthm = 0)
{
    Printer printer;
    return verifyproofsteps(steps, printer, pthm);
}
// Verify a regular proof. The "proof" argument should be a non-empty sequence
// of valid labels. Return the statement the "proof" proves.
// Return the empty expression if the "proof" is invalid.
Expression verifyregularproof
    (strview label, class Database const & database,
     Proof const & proof, Hypotheses const & hypotheses);

// Return if "conclusion" == "expression" to be proved.
bool provesrightthing
    (strview label,
     Expression const & conclusion, Expression const & expression);

#endif // VERIFY_H_INCLUDED
