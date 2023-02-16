#include <algorithm>
#include <limits>
#include "util.h"
#include "util/arith.h"
#include "util/filter.h"
#include "util/find.h"
#include "util/for.h"
#include "util/progress.h"
#include "util/timer.h"
#include "def.h"            // checkdefinitions
#include "database.h"
#include "io.h"
#include "proof/analyze.h"  // checkrPolish
#include "proof/verify.h"   // verifyregularproof
#include "propctor.h"       // checkpropsat
#include "stat.h"           // isasshard
#include "syntaxiom.h"      // checkrPolish

template<class T> static T testlog2()
{
    T const static digits(std::numeric_limits<T>::digits);
    for (T d(0), n(1); d < digits; ++d, n *= 2)
        if (log2(n) != d)
            return std::cout << log2(n) << d, n;

    return 0;
}

bool testlog2()
{
    const char static * const types[] = {"short", "int", "long"};
    const unsigned long results[] =
    {testlog2<unsigned short>(),testlog2<unsigned>(),testlog2<unsigned long>()};
    const unsigned n(sizeof(results) / sizeof(unsigned long));
    const unsigned i(std::find_if(results, util::end(results),
                    std::negate<unsigned long>()) - results);
    if (i != n)
        std::cerr << "ERROR: " << types[i] << ' ' << results[i] << std::endl;
    return i != n; // should be 0
}

template<template<bool, class> class Filter, bool B, class Container, class A>
static bool checkfilter
    (Filter<B, Container> const & filter, A const & alien)
{
    Container const & container(filter.container);

    std::size_t const count
        (std::count_if(container.begin(), container.end(), filter));
    if (count < container.size())
    {
        std::cerr << "Only " << count << " items out of "
                  << container.size() << " found by filter" << std::endl;
        return false;
    }

    if (filter(alien))
    {
        std::cerr << "Alien item found by filter" << std::endl;
        return false;
    }

    return true;
}

bool testfilter(unsigned n)
{
    std::cout << "Checking filters" << std::endl;
    if (n == 0) n = 1;

    std::vector<unsigned> v;
    for (unsigned i(0); i <= n; v.push_back(++i))
    {
        if (!checkfilter(util::filter(v), 0))
        {
            std::cerr << "failed with " << i << " elements" << std::endl;
            return false;
        }
    }

    return true;
}

// Check if iterators to assertions are sorted in ascending number.
bool checkassiters
    (Assertions const & assertions, Assiters const & assiters)
{
    for (Assiter iter(assertions.begin()); iter != assertions.end(); ++iter)
    {
        Assertions::size_type const n(iter->second);

        if (n > assertions.size())
        {
            std::cerr << "Assertion " << iter->first << "'s number " << n
                      << " is larger than the total number "
                      << assertions.size() << std::endl;
            return false;
        }

        if (assiters[n] != iter)
        {
            std::cerr << "Assertion " << iter->first << "'s number " << n
                      << " != its position " << util::find(assiters, iter)
                      - assiters.begin() << std::endl;
            return false;
        }
    }

    return true;
}

// Check if all the definitions are syntactically okay.
bool Database::checkdefinitions() const
{
    FOR (Definitions::const_reference r, definitions())
    {
//std::cout << "Checking definition for " << r.first << std::endl;
        Definition const & definition(r.second);
        // Points to the corresponding assertion.
        Assptr pdef(definition.pdef);
        if (unexpected(!pdef, "empty definition for", r.first))
            continue;
        // Construct the statement from the revPolish notation.
        Proofsteps steps(definition.lhs);
        steps += definition.rhs;
        steps.push_back(pdef->second.exprPolish.back());
        Expression const & result(verifyproofsteps(steps, pdef));
        // Check if it agrees with the expression.
        Expression const & exp(pdef->second.expression);
        bool okay(!result.empty() && result[0]==typecodes().normalize(exp[0]));
        okay &= util::equal(result.begin() + 1, result.end(),
                            exp.begin() + 1, exp.end());
        if (!okay)
        {
            std::cerr << exp << "Def syntax proof shows\n" << result;
            return false;
        }
    }

    return true;
}

// Determine if proof is the rPolish notation for the expression of ass.
// Also check splitting of the proof with respect to splitters, imps and ands.
static bool checkrPolish
    (strview label, Assertion const & ass, Proofsteps const & rPolish,
     Proof const & splitters,
     Proof const & imps = Proof(), Proof const & ands = Proof())
{
    Prooftree const & tree(prooftree(rPolish));
    Subprfsteps subprfsteps;
    prealloc(subprfsteps, ass.varsused);
    bool const ok(findsubstitutions(rPolish, tree, rPolish, tree, subprfsteps));
    if (unexpected(!ok, "failed unification test for", rPolish))
        return false;

    // Storage for hex names, because hex() points to static storage
    static Proof hexnums;
    for (Proofsize i(hexnums.size()); i < rPolish.size(); ++i)
        hexnums.push_back(util::hex(i));
    // Split the proof.
    Subprfs subproofs;
    Proof proof(rPolish.begin(), rPolish.end());
//std::cout << "Syntax proof: " << proof;
    Proof const & split(splitproof(proof, tree, splitters, subproofs));
//std::cout << "Split syntax proof: " << split;
    // Prepare hypotheses.
    Hypotheses hyps;
    extern Database database;
    for (Proofsize i(0); i < subproofs.size(); ++i)
    {
        Hypothesis & hyp(hyps[hexnums[i]]);
        Proof const subproof(subproofs[i].first, subproofs[i].second);
//std::cout << "Subproof: " << subproof;
        hyp.first = verifyregularproof(label, database, subproof,
                                       database.hypotheses());
//std::cout << "Subexpression " << hyp.first << " added.\n";
        hyp.second = false;
    }
    // Reassemble and check the proof.
    Expression const & exp(verifyregularproof(label, database, split, hyps));
    if (!provesrightthing(label, exp, ass.expression))
        return false;

    if (imps.empty() && ands.empty())
        return true; // No need to split assumptions

    return !splitassumptions(proof, tree, imps, ands).empty();
}

// Check the syntax of an assertion (& all hypotheses). Return true iff okay.
bool Syntaxioms::checkrPolish
    (strview label, Assertion ass, struct Typecodes const & typecodes) const
{
    Expression & exp(ass.expression);
    std::deque<Hypiter> const & hyps(ass.hypotheses);

    if (exp.empty())
        return false;
    exp[0] = typecodes.normalize(exp[0]);
    static const Proof splitters(1, "wb");
//std::cout << ass.exprPolish;
    if (!::checkrPolish(label, ass, ass.exprPolish, splitters))
        return false;

    std::vector<Proofsteps> const & hypproofs(ass.hypsrPolish);
    std::vector<Proofsteps>::const_iterator iterproof(hypproofs.begin());

    for (Hypiteriter iterhyp(hyps.begin()); iterhyp != hyps.end();
        ++iterhyp, ++iterproof)
    {
        if ((*iterhyp)->second.second)
            continue; // Floating hypothesis
        exp = (*iterhyp)->second.first;
        if (exp.empty())
            return false;
//std::cout << "Checking hypothesis " << (*iterhyp)->first << ": " << exp;
        exp[0] = typecodes.normalize(exp[0]);
        if (!::checkrPolish(label, ass, *iterproof, splitters))
            return false;
    }

    return true;
}

// Test syntax parser. Return 1 iff okay.
bool Database::checkrPolish() const
{
    std::cout << "Checking syntax trees";
    Progress progress;
    Timer timer;
    Assertions::size_type count(0), all(assertions().size());
    FOR (Assertions::const_reference ass, assertions())
    {
        if (!syntaxioms().checkrPolish(ass.first, ass.second, typecodes()))
        {
            printass(ass, count);
            std::cerr << "\nSyntax error!" << std::endl;
            return false;
        }
        progress << ++count/static_cast<double>(all);
    }

    std::cout << "done in " << timer << 's' << std::endl;
    return true;
}

// Check the syntax of the theorem (& all hypotheses). Return true iff okay.
bool Database::checkpropassertion() const
{
    std::cout << "Checking ";
    Timer timer;
    Assertions::size_type count(0), all(assvec().size());
    for (Assiters::size_type i(1); i < all; ++i)
    {
        Assiter const iter(assvec()[i]);
        Assertion const & assertion(iter->second);
        if (assertion.expression.empty())
            return false;
        if (typecodes().isprimitive(assertion.expression[0]))
            continue; // Skip syntax axioms.
        if (!(assertion.type & Assertion::PROPOSITIONAL))
            continue; // Skip non propositional assertions.

        ++count;
        if (!propctors().checkpropsat(iter->second, iter->second.exprPolish))
        {
            printass(*iter);
            std::cerr << "Logic error!" << std::endl;
            return false;
        }
    }

    std::cout << count << '/' << all - 1 << " propositional assertions in ";
    std::cout << timer << 's' << std::endl;
    return true;
}

// Print all hard assertions.
void Database::printhardassertions() const
{
    for (Assiters::size_type i(1); i < assvec().size(); ++i)
    {
        Assiter iter(assvec()[i]);
        if (isasshard(iter->second, syntaxioms()))
            printass(*iter), std::cout << std::endl;
    }
}
