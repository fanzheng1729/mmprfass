#ifndef PROPCTOR_H_INCLUDED
#define PROPCTOR_H_INCLUDED

#include <iosfwd>
#include "cnf.h"
#include "def.h"
#include "proof/step.h"

// Propositional syntax constructor
struct Propctor: Definition
{
    // Truth table of the propositional connective
    Bvector truthtable;
    // CNF clauses reflecting the truth table
    CNFClauses cnf;
    // # arguments of the propositional connective
    Atom argcount;
};

std::ostream & operator<<(std::ostream & out, Propctor const & propctor);

// Check if the data of a propositional syntax constructor is okay.
bool checkpropctor(Propctor const & propctor);

// Map: propositional syntax constructor label -> data
struct Propctors : public std::map<strview, Propctor>
{
    Propctors(Definitions const & definitions = Definitions());
// Check if data for all propositional syntax constructor are okay.
    bool okay(Definitions const & definitions = Definitions()) const;
// Add CNF clauses from reverse Polish notation.
// # of auxiliary atoms start from atomcount.
// Return true if okay. First auxiliary atom = hyps.size()
    bool addcnffromrPolish
        (Proofsteps const & proofsteps, std::deque<Hypiter> const & hyps,
         CNFClauses & cnf, Atom & atomcount) const;
// Translate the hypotheses of a propositional assertion to the CNF of an SAT.
    CNFClauses hypcnf(struct Assertion const & ass, Atom & count) const;
// Translate a propositional assertion to the CNF of an SAT instance.
    CNFClauses cnf
        (struct Assertion const & ass, Proofsteps const & proofsteps) const;
// Check if an expression is valid given a propositional assertion.
    bool checkpropsat
        (struct Assertion const & ass, Proofsteps const & proofsteps) const;
private:
// Initialize with basic propositional connectives.
    void init();
// Add a definition. Return the iterator to the entry. Otherwise return end.
    Propctors::const_iterator adddef
        (Definitions const & definitions, Definitions::const_reference labeldef);
// Evaluate *iter at arg. Return -1 if not okay.
    int calctruthvalue
        (Definitions const & definitions, Proofsteps const & lhs,
         Proofsteps const & rhs, Bvector::size_type arg);
};

#endif // PROPCTOR_H_INCLUDED
