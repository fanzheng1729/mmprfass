#ifndef DEFN_H_INCLUDED
#define DEFN_H_INCLUDED

#include "ass.h"
#include "util/filter.h"
#include "util/for.h"
#include "proof/step.h"

// A definition
struct Definition
{
    // Pointer to the defining assertion, null -> no definition
    Assptr pdef;
    // Left and right hand side of definition
    Proofsteps lhs, rhs;
    // # of the defining assertion
    operator Assertions::size_type() const { return pdef->second.number; }
    Definition() : pdef(NULL) {}
    // Find the revPolish notation of (LHS, RHS).
    Definition(Assertions::const_reference rass);
    Definition
        (Assertions::const_reference rass, struct Typecodes const & typecodes,
         Equalities const & equalities);
    // Check if the defined syntax appears on the RHS (rule 2).
    bool iscircular() const { return util::filter(rhs)(lhs.back()); }
    // Check if a variable is dummy, i.e., does not appear on the LHS.
    bool isdummy(Symbol3 var) const
    { return !util::filter(lhs)(Proofstep(var.phyp)); }
    // Check the required disjoint variable hypotheses (rules 3 & 4).
    bool checkdv() const;
    // Check if all dummy variables are bound (not fully implemented).
    bool checkdummyvar(struct Typecodes const & typecodes) const;
};

// Map: label of syntax axiom -> its definition
struct Definitions : std::map<const char *, Definition>
{
    Definitions() {}
    Definitions
        (Assertions const & assertions, struct Commentinfo const & commendinfo,
         Equalities const & equalities);
private:
    // Add a definition. Return true iff okay.
    bool adddef
        (Assertions::const_reference rass, struct Typecodes const & typecodes,
         Equalities const & equalities);
};

// Return propositional definitions in assertions.
Definitions propdefinitions
    (Assertions const & assertions,
     struct Commentinfo const & commentinfo, Equalities const & equalities);

#endif // DEFN_H_INCLUDED
