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
static Proofsize argssize
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

// Adds a generated term.
struct Termadder : Adder
{
    Terms & terms;
    Proofstep root;
    Termadder(Terms & terms, Proofstep root) : terms(terms), root(root) {}
    virtual void operator()(Argtypes const & types, Genresult const & result,
                            Genstack const & stack)
    {
        terms.push_back(writerPolish(types, result, stack, root));
    }
};

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

// Generate all terms for all arguments with rPolish up to a given size.
void dogenerate
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     Argtypes const & argtypes, Proofsize size,
     Genresult & result, Termcounts & termcounts, Adder & adder)
{
    // Stack of terms to be tried
    Genstack stack;
    // Preallocate for efficiency.
    Proofsize const argcount(argtypes.size());
    stack.reserve(argcount);
    do
    {
        if (stack.size() < argcount) // Not all arguments seen
        {
            strview type(argtypes[stack.size()]);
            generateupto(varsused, syntaxioms, type, size - 1, result, termcounts);
            if (result[type].empty()) // No term generated
                break;

            if (argcount - stack.size() > 1) // At least 2 arguments not seen
                stack.push_back(0);
            else
            {
                // Size of the only unseen argument
                Proofsize const lastsize(size - 1 -
                                         argssize(argtypes, result, stack));
                // 1st substitution with that size
                stack.push_back(termcounts[type][lastsize - 1]);
            }
            continue;
        }
        // All arguments seen. Write rPolish of term.
        adder(argtypes, result, stack);
//std::cout << "New term: " << terms.back();
        // Try the next substitution.
        while (!stack.empty())
        {
            // Check if the size of the last substitution is maximal.
            if (argssize(argtypes, result, stack)
                < size - 1 - argcount + stack.size())
                break;
            // Type of the last substitution
            strview type(argtypes[stack.size() - 1]);
            // Index of the last substitution
            Terms::size_type const index(stack.back());
            if (index < termcounts[type][result[type][index].size()] - 1)
                break;
            stack.pop_back();
        }
        if (!stack.empty())
            ++stack.back();
        else
            break;
    } while (!stack.empty());
}

void generateupto
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     strview type, Proofsize size, Genresult & result,
     Termcounts & termcounts)
{
//std::cout << "Generating up to size " << size << std::endl;
    Terms & terms(result[type]);
    Termcounts::mapped_type & countbysize(termcounts[type]);
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

    generateupto(varsused, syntaxioms, type, size - 1, result, termcounts);

    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
//std::cout << "Syntaxiom " << syntaxiom.first << std::endl;
        Assertion const & ass(syntaxiom.second.assiter->second);
        if (ass.exprPolish.size() <= 1 || ass.exprPolish.size() > size ||
            ass.expression[0] != type)
            continue;

        Argtypes const & types(argtypes(ass.exprPolish));
        if (types.empty())
            continue; // Bad syntax axiom.

        // Callback functor to add terms
        Termadder adder(terms, ass.exprPolish.back());
        // Main loop of term generation
        dogenerate(varsused, syntaxioms, types, size,
                   result, termcounts, adder);
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
        if (syntaxiom.second.assiter->second.number <= assertion.number)
            filtered.insert(syntaxiom);

    return generate(assertion.varsused, filtered, type, size);
};
