#ifndef PROOF_H_INCLUDED
#define PROOF_H_INCLUDED

#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "strview.h"

// Boolean vector
typedef std::vector<bool> Bvector;
// Set of constants
typedef std::set<strview> Constants;

// A constant or a variable with ID and pointer to defining hypothesis
struct Symbol3;
// Expression is a sequence of math tokens.
typedef std::vector<Symbol3> Expression;
// Iterator to an expression
typedef Expression::const_iterator Expiter;
// Begin and end iterators to a subexpression
typedef std::pair<Expiter, Expiter> Subexp;
// A sequence of subexpressions
typedef std::vector<Subexp> Subexps;

// The first parameter is the statement of the hypothesis, the second is
// true iff the hypothesis is floating.
typedef std::pair<Expression, bool> Hypothesis;
// Map: label -> hypothesis
typedef std::map<strview, Hypothesis> Hypotheses;
// Pointer to (label, hypothesis)
typedef Hypotheses::const_pointer Hypptr;

// A constant or a variable with ID
struct Symbol2 : strview
{
    // ID of variable, 0 for constants
    typedef std::size_t ID;
    ID id;
    operator ID() const { return id; }
    Symbol2(strview str = "", ID n = 0) : strview(str), id(n) {}
};
// A constant or a variable with ID and pointer to defining hypothesis
struct Symbol3 : Symbol2
{
    // Pointer to the floating hypothesis for variable, NULL for constant
    Hypptr phyp;
    strview typecode() const { return phyp->second.first[0]; }
    Symbol3(strview str = "", ID n = 0, Hypptr p = NULL) :
        Symbol2(str, n), phyp(p) {}
};
// Functor checking if a symbol is a constant or a variable
static const std::logical_not<Symbol2::ID> isconst;
static const std::negate<Symbol2::ID> isvar;

// Map: var -> id
typedef std::map<strview, Symbol3::ID> VarIDmap;
// Set of symbols
typedef std::set<Symbol2> Symbol2s;
typedef std::set<Symbol3> Symbol3s;
// Map: var -> is used in (hypotheses..., expression)
typedef std::map<Symbol3, Bvector> Varsused;
inline bool operator<(Varsused::const_reference var1, Varsused::const_reference var2)
{ return var1.first.id < var2.first.id; }
// Set: (x, y) = $d x y
typedef std::set<std::pair<Symbol2, Symbol2> > Disjvars;

typedef std::size_t Proofnumber;
// Vector of proof numbers
typedef std::vector<Proofnumber> Proofnumbers;
// Proof is a sequence of labels.
typedef std::vector<std::string> Proof;
// Iterator to a proof
typedef Proof::const_iterator Prfiter;

// An axiom or a theorem
struct Assertion;
// Map: label -> assertion
typedef std::map<strview, Assertion> Assertions;
// Pointer to (label, assertion)
typedef Assertions::const_pointer Assptr;
// Iterator to an assertion
typedef Assertions::const_iterator Assiter;
// Vector of iterators to assertions
typedef std::vector<Assiter> Assiters;

// Iterator to a hypothesis
typedef Hypotheses::const_iterator Hypiter;
// Check if the name of the hypothesis pointed to is label.
inline bool operator==(Hypiter iter, strview label) {return iter->first==label;}
// A sequence of hypothesis iterators
typedef std::vector<Hypiter> Hypiters;
// # of hypotheses
typedef Hypiters::size_type Hypsize;

// Append an expression to another.
template<class T, class U>
std::vector<T> & operator+=(std::vector<T> & a, std::vector<U> const & b)
{
    a.reserve(b.end() - b.begin() + a.size());
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

#endif // PROOF_H_INCLUDED
