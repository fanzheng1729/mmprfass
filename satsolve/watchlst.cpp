#include "../io.h"
#include "../util/for.h"
#include "watchlst.h"

// Set up the watch list from cnf.
// Set up the empty list if cnf has an empty clause.
Watchlist_solver::Watchlist::Watchlist(CNFClauses const & cnf)
{
    if (cnf.hasemptyclause()) return;
    // One watch list for each literal (atom or ~atom)
    resize(cnf.atomcount() * 2);
    if (empty())
        std::cerr << "ERROR: empty watch list for CNF:\n" << cnf;
    // Each clause watches its first literal.
    FOR (CNFClause const & clause, cnf)
        (*this)[clause[0]].push_back(&clause - cnf.data());
}

// Updates the watch list after literal falseliteral is assigned FALSE
// by making any clause watching it watch something else.
// Return false if it is impossible to do zo, meaning a clause is contradicted.
bool Watchlist_solver::Watchlist::update
    (CNFClauses const & cnf, Literal falseliteral,
     CNFModel const & model)
{
    if (unexpected(falseliteral >= size(), "literal", falseliteral))
        return false;

    // List of clauses watching false literal
    value_type & clauses((*this)[falseliteral]);

    // Iterate over such clauses.
    for (value_type::reverse_iterator
         iter(clauses.rbegin()); iter != clauses.rend(); ++iter)
    {
        // Let clause be such a clause.
        CNFClauses::const_reference clause(cnf[*iter]);
        // Indicates whether another watch literal in clause is found.
        bool foundalternative(false);
        // Point alter to a literal in clause.
        for (CNFClause::size_type i(0); i < clause.size(); ++i)
        {
            Literal alter(clause[i]);
            // atom = alter / 2, sense = alter % 2
            // sense = 0, literal is positive, model != false
            // sense = 1, literal is negative, model != true
            if (model[alter / 2] != static_cast<CNFTruthvalue>(alter % 2))
            {
                // Found another literal alter to watch.
                foundalternative = true;
                // Let the clause *iter watch alter.
                (*this)[alter].push_back(*iter);
                break;
            }
        }

        if (!foundalternative)
        {
//std::cout << "Clause " << *iter << " contradicted.
            // Let clause *iter still watch the old atom.
            // It will be either TRUE or NONE in the next iteration.
            clauses.erase(iter.base(), clauses.end());
            return false;
        }
    }

    // Now no clause is watching falseliteral.
    clauses.clear();
    return true;
}
