#ifndef DPLL_H_INCLUDED
#define DPLL_H_INCLUDED

#include "../sat.h"

// signed literal: P = 1, !P = -1, Q = 2, !Q = -2, ...
typedef int sLiteral;
// Translation from unsigned to signed literals
inline sLiteral sliteral(Literal lit)
{ return lit % 2 ? -(lit / 2) - 1 : lit / 2 + 1; }
// A list of signed literals
typedef std::vector<sLiteral> sCNFClause;
// Instance in conjunctive normal form
typedef std::vector<sCNFClause> sCNF;

// The following is from https://github.com/necavit/li-sat-solver
/**
 * Reads the CNF and initializes
 * any remaining necessary data structures and variables.
 */
void parseInput(CNFClauses const & cnf);

/**
 * Checks for any unit clause and sets the appropriate values in the
 * model accordingly. If a contradiction is found among these unit clauses,
 * early failure is triggered.
 */
bool checkUnitClauses();

/**
 * Executes the DPLL (Davis�Putnam�Logemann�Loveland) algorithm, performing a full search
 * for a model (interpretation) which satisfies the formula given as a CNF clause set.
 */
bool doDPLL();

class DPLL_solver : public Satsolver
{
public:
    DPLL_solver(CNFClauses const & cnf) : Satsolver(cnf) { parseInput(cnf); }
    bool sat() const { return checkUnitClauses() and doDPLL(); }
};

#endif // DPLL_H_INCLUDED
