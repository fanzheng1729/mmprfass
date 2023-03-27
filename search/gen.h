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

// Virtual base class for adder
struct Adder
{
    virtual void operator()(Argtypes const & types, Genresult const & result,
                            Genstack const & stack) = 0;
};

// Generate all terms for all arguments with rPolish up to a given size.
void dogenerate
    (Varsused const & varsused, struct Syntaxioms const & syntaxioms,
     Proofsize argcount, Argtypes const & argtypes, Proofsize size,
     Genresult & result, Termcounts & counts, Adder & adder);

#endif // GEN_H_INCLUDED
