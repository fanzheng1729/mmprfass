#ifndef GEN_H_INCLUDED
#define GEN_H_INCLUDED

#include "../proof/step.h"

// vector of generated terms
typedef std::vector<Proofsteps> Terms;
// map: type -> terms
typedef std::map<strview, Terms> Genresult;
// Counts[type][i] = # of terms up to size i
typedef std::map<strview, std::vector<Terms::size_type> > Termcounts;
// Argtypes[i] = typecode of argument i
typedef std::vector<strview> Argtypes;
// Genstack[i] = # of terms substituted for argument i
typedef std::vector<Terms::size_type> Genstack;

// Generate all terms with rPolish up to a given size.
void generateupto
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     strview type, Proofsize size, Genresult & result,
     Termcounts & counts);

// Generate all terms whose rPolish is of a given size.
Terms generate
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     strview type, Proofsize size);

Terms generate(struct Assertion const & assertion,
               struct Syntaxioms const & syntaxioms,
               strview type, Proofsize size);

// Return the sum of the sizes of substituted arguments.
Proofsize argssize
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack);

// Main loop of term generation
template<class T>
void dogenerate
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     Proofsize argcount, Argtypes const & argtypes, Proofsize size,
     Genresult & result, Termcounts & counts, T & adder)
{
    // Stack of terms to be tried
    Genstack stack;
    // Preallocate for efficiency.
    stack.reserve(argcount);
    do
    {
        if (stack.size() < argcount) // Not all arguments seen
        {
            strview type(argtypes[stack.size()]);
            generateupto(varsused, syntaxioms, type, size - 1, result, counts);
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
                stack.push_back(counts[type][lastsize - 1]);
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
            if (index < counts[type][result[type][index].size()] - 1)
                break;
            stack.pop_back();
        }
        if (!stack.empty())
            ++stack.back();
        else
            break;
    } while (!stack.empty());
}

#endif // GEN_H_INCLUDED
