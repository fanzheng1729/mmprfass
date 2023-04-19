#ifndef CNF_H_INCLUDED
#define CNF_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>
#include "util/for.h"
#include "util/filter.h"

// Atom: P = 0, Q = 1, ... Literal: P = 0, !P = 1, Q = 2, !Q = 3, ...
typedef std::size_t Atom, Literal;
// Boolean vector
typedef std::vector<bool> Bvector;
// A list of literals
typedef std::vector<Literal> CNFClause;

// Satisfaction of a clause
enum CNFClausesat{UNDECIDED = -2, UNIT = -1, CONTRADICTORY = 0, SATISFIED = 1};
// Sense of an individual atom
enum CNFTruthvalue{CNFNONE = -1, CNFFALSE = 0, CNFTRUE = 1};
// Model of an instance
struct CNFModel : public std::vector<int>
{
    CNFModel(size_type const n) : std::vector<int>(n, CNFNONE) {}
    // sense = 0, literal is positive, assign true;
    // sense = 1, literal is negative, assign false.
    void assign(Literal const lit) { (*this)[lit / 2] = int(1 - lit % 2); }
    // Return the sense of a literal.
    int test(Literal const lit) const
    {
        return (*this)[lit / 2] == CNFNONE ? static_cast<int>(CNFNONE) :
            (*this)[lit / 2] ^ (lit % 2);
    }
};

// Check the satisfaction of clause under the model.
// If UNIT, return (UNIT, index of unassigned literal).
// If UNDECIDED, return (UNDECIDED, index of unassigned literal).
std::pair<CNFClausesat, CNFClause::size_type> CNFclausesat
        (CNFClause const & clause, CNFModel const & model);

// Instance in conjunctive normal form
struct CNFClauses : public std::vector<CNFClause>
{
    CNFClauses(size_type n = 0) : std::vector<CNFClause>(n) {}
// Construct the cnf representing a truth table.
    CNFClauses(Bvector const & truthtable);
    bool hasemptyclause() const { return util::filter(*this)(CNFClause()); }
// Return # atoms in cnf. Return 1 for empty instance.
    Atom atomcount() const
    {
        // Maximal literal
        Literal max(0);
        FOR (const_reference clause, *this)
        {
            if (clause.empty()) continue;
            max = std::max(max,*std::max_element(clause.begin(),clause.end()));
        }
        // Maximal atom + 1
        return max / 2 + 1;
    }
// Append cnf to the end.
// If atom < argcount, change it to arglist[atom], with sense adjusted.
// If atom >= argcount, change it to new atoms starting from atomcount.
// argcount and arglist are separate to work with stack based arguments.
    void append
        (CNFClauses const & cnf, Atom const atomcount,
         Literal const * const arglist, Atom const argcount);
// Add a clause containing a single literal/the next atom alone or its negation.
    void closeoff(Literal lit) { push_back(CNFClause(1, lit)); }
    void closeoff(bool negate = false) { closeoff((atomcount()-1)*2+negate); }
// Return if there is no contradiction in the model so far.
    bool okaysofar(CNFModel const & model) const
    {
        FOR (const_reference clause, *this)
            if (CNFclausesat(clause, model).first == CONTRADICTORY)
                return false;

        return true;
    }
// Move clause to the next UNIT, CONTRADICTORY or UNDECIDED clause.
// Return (Clausesat, index of unassigned literal).
    std::pair<CNFClausesat, CNFClause::size_type> nextclause
        (size_type & clause, CNFModel const & model) const;
// Propagate unit clauses iteratively.
// Return (TRUE, 0), (FALSE, CONTRADICTORY clause) or (NONE, UNDECIDED clause).
    template<class T> std::pair<CNFTruthvalue, CNFClauses::size_type>
        unitprop(CNFModel & model, T callback) const;
// Propagate unit clauses iteratively.
// Return (TRUE, 0), (FALSE, CONTRADICTORY clause) or (NONE, UNDECIDED clause).
    std::pair<CNFTruthvalue, CNFClauses::size_type>
        unitprop(CNFModel & model) const
    {
        void (*p)(size_type, CNFClause::size_type) = 0;
        return unitprop(model, p);
    }
    struct Unitpropcallback { operator bool() const {return true;} };
// Return if the clauses are satisfiable.
    bool sat() const;
// Map: free atoms -> truth value.
// Return the empty vector if not okay.
    Bvector truthtable(Atom const nfree) const;
};

template<class T> std::pair<CNFTruthvalue, CNFClauses::size_type>
    CNFClauses::unitprop(CNFModel & model, T callback) const
{
    if (empty())
        return std::make_pair(CNFTRUE, 0u);

    size_type clause(0);
    while (true)
    {
        std::pair<CNFClausesat, CNFClause::size_type> const result
            (nextclause(clause, model));

        switch (result.first)
        {
        case SATISFIED:
            return std::make_pair(CNFTRUE, 0u);
        case CONTRADICTORY:
            return std::make_pair(CNFFALSE, clause);
        case UNDECIDED:
            return std::make_pair(CNFNONE, clause);
        case UNIT: ;
        }
        // Found a unit clause. Assign a new literal.
        if (callback)
            callback(clause, result.second);
        model.assign((*this)[clause][result.second]);
        // Move to next clause.
        ++clause %= size();
    }
}

// Pair (CNF, # clauses corresponding to hypotheses)
typedef std::pair<CNFClauses, std::vector<CNFClauses::size_type> > Hypscnf;

#endif // SAT_H_INCLUDED
