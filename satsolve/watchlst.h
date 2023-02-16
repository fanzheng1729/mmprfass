#ifndef WATCHLST_H_INCLUDED
#define WATCHLST_H_INCLUDED

#include "../sat.h"

class Watchlist_solver : public Satsolver
{
public:
    Watchlist_solver(CNFClauses const & cnf) : Satsolver(cnf) {}
    bool sat()
    {
        // Set up the watch list.
        Watchlist watchlist(rcnf);
        // If the SAT instance has empty clause, it is NOT satisfiable.
        return watchlist.empty() ? false : backtrack_sat(watchlist);
    }
private:
// Clauses that watch a given literal
    class Watchlist : public std::vector<std::vector<CNFClauses::size_type> >
    {
    public:
    // Set up the watch list from cnf.
    // Set up the empty list if cnf has an empty clause.
        Watchlist(CNFClauses const & cnf);
    // Updates the watch list after literal falseliteral is assigned FALSE
    // by making any clause watching it watch something else.
    // Return false if it is impossible to do zo, meaning a clause is contradicted.
        bool update(CNFClauses const & cnf, Literal falseliteral,
                    CNFAssignment const & assignment);
        bool operator()
            (CNFClauses const & cnf, CNFAssignment const & assignment,
             Atom atom)
        {
            return update(cnf, atom * 2 + assignment[atom], assignment);
        }
    };
};

#endif // WATCHLST_H_INCLUDED
