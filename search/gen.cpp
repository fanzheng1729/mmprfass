#include "gen.h"
#include "../syntaxiom.h"
#include "../util/for.h"
#include "../util/iter.h"

// Return the typecodes of the arguments of a syntax axiom.
static Argtypes argtypes(Proofsteps const & assrPolish)
{
    if (assrPolish.empty())
        return Argtypes();

    Argtypes result;
    // Preallocate for efficiency.
    result.reserve(assrPolish.size() - 1);

    for (Proofsize i(0); i < assrPolish.size() - 1; ++i)
    {
        if (assrPolish[i].type != Proofstep::HYP)
            return Argtypes();
        result.push_back(assrPolish[i].phyp->second.first[0]);
    }

    return result;
}

// Return the sum of the sizes of substituted arguments.
Proofsize argssize
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack)
{
    Proofsize count(0);
    for (Genstack::size_type i(0); i < stack.size(); ++i)
        count += result.at(argtypes[i])[stack[i]].size();

    return count;
}

static Proofsteps writerPolish
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack, Proofstep const & root)
{
    Proofsteps rPolish;
    // Preallocate for efficiency.
    rPolish.reserve(argssize(argtypes, result, stack));

    for (Genstack::size_type i(0); i < stack.size(); ++i)
        rPolish += result.at(argtypes[i])[stack[i]];
    rPolish.push_back(root);

    return rPolish;
}

// Generate all terms of size 1.
static Terms generateupto1
    (Varsused const & varsused, Syntaxioms const & syntaxioms, strview type)
{
    Terms terms;

    // Preallocate for efficiency.
    terms.reserve(varsused.size());
    FOR (Varsused::const_reference var, varsused)
        if (var.first.typecode() == type)
            terms.push_back(Proofsteps(1, Proofstep(var.first.phyp)));

    // Generate all 1-step syntax axioms.
    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
        Assertion const & ass(syntaxiom.second.assiter->second);
        if (ass.exprPolish.size() == 1 && ass.expression[0] == type)
            terms.push_back(ass.exprPolish);
    }

    return terms;
}

// Adds a generated term.
struct Termadder
{
    Terms & terms;
    Proofstep const & root;
    void operator()(Argtypes const & types, Genresult const & result,
                    Genstack const & stack)
    {
        terms.push_back(writerPolish(types, result, stack, root));
    }
};

void generateupto
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     strview type, Proofsize size, Genresult & result,
     Termcounts & counts)
{
//std::cout << "Generating up to size " << size << std::endl;
    Terms & terms(result[type]);
    Termcounts::mapped_type & countbysize(counts[type]);
    // Preallocate for efficiency.
    countbysize.reserve(size + 1);

    if (countbysize.empty())
    {
        terms = generateupto1(varsused, syntaxioms, type);
        // Record # of size 1 terms.
        countbysize.resize(2);
        countbysize[1] = terms.size();
    }

    if (countbysize.size() >= size + 1)
        return;

    generateupto(varsused, syntaxioms, type, size - 1, result, counts);

    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
//std::cout << "Syntaxiom " << syntaxiom.first << std::endl;
        Assertion const & ass(syntaxiom.second.assiter->second);
        if (ass.exprPolish.size() <= 1 || ass.exprPolish.size() > size ||
            ass.expression[0] != type)
            continue;

        Argtypes const types(argtypes(ass.exprPolish));
        if (types.empty())
            continue; // Bad syntax axiom.

        // Callback functor to add terms
        Termadder adder = {terms, ass.exprPolish.back()};
        // # of arguments, at least 1
        Proofsize const argcount(ass.exprPolish.size() - 1);
        // Main loop of term generation
        dogenerate(varsused, syntaxioms, argcount, types, size,
                   result, counts, adder);
    }
//std::cout << terms.size() << " terms generated" << std::endl;
    // Record the # of terms.
    countbysize.insert(countbysize.end(),
                       size + 1 - countbysize.size(), terms.size());
}

// Generate all terms whose rPolish is of a given size.
Terms generate
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     strview type, Proofsize size)
{
    if (size == 0)
        return Terms();

    Genresult result;
    Termcounts counts;
    generateupto(varsused, syntaxioms, type, size, result, counts);

    return Terms(result[type].begin() + counts[type][size - 1],
                 result[type].end());
}

Terms generate(struct Assertion const & assertion,
               struct Syntaxioms const & syntaxioms,
               strview type, Proofsize size)
{
    Syntaxioms filtered;
    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
        if (syntaxiom.second.assiter->second <= assertion)
            filtered.insert(syntaxiom);

    return generate(assertion.varsused, filtered, type, size);
};
