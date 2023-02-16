#include "CDCL.h"
#include "../../io.h"
#include "../../msg.h"
#include "../../util/filter.h"
#include "../../util/for.h"

CNFAssignment CDCL_solver::Decision_list::toassignment(Atom atomcount) const
{
    CNFAssignment assignment(atomcount);
    FOR (value_type decision, *this)
        assignment.assign(decision.first / 2);

    return assignment;
}

// Return last propagated decision.
CDCL_solver::Decision_list::const_reverse_iterator
    CDCL_solver::Decision_list::lastunitprop(CNFClause const & clause) const
{
    for (const_reverse_iterator iter(rbegin()); iter != rend(); ++iter)
    {
        if (!istrial(*iter) && util::filter(clause)(iter->first / 2 ^ 1))
                return iter; // Found
    }
    // Not found
    return rend();
}

bool CDCL_solver::Decision_list::movetonext(CNFClauses::size_type badindex)
{
    // Find the last trial decision.
    reverse_iterator const iter(std::find_if(rbegin(), rend(), istrial));
//std::cout << "Erase " << iter.base() - begin() << std::endl;
    // If there is no trial decision, the CNF is UNSATISFIABLE.
    if (iter == rend()) return false;
    // Remove all decisions after it.
    erase(iter.base(), end());
    // Flip the lit, make it non-trial, and record the propagating clause.
    back().first ^= 3;
    back().second = badindex;
//std::cout << "Push  " << back().first << "," << back().second << std::endl;
    return true;
}

CNFClauses::size_type CDCL_solver::learn
    (CNFClauses::size_type badindex, Decision_list const & dlist)
{
    while (1)
    {
        // Last propagated decision in the conflicting clause
        Decision_list::const_reverse_iterator const iter
            (dlist.lastunitprop(extcnf[badindex]));
        if (iter == dlist.rend())
            return badindex; // All lits in the conflicting clause are trial.

        // Create learned clause.
        extcnf.resize(extcnf.size() + 1);
        Literal const lit(iter->first / 2);
//std::cout << "Last propagated literal: " << lit << std::endl;
        CNFClause const & badcl(extcnf[badindex]);
//std::cout << "conflicting clause #" << badindex << std::endl;
        CNFClause const & clause(extcnf[iter->second]);
//std::cout << "Propagating clause #: " << iter->second << std::endl;
        CNFClause & learned(extcnf.back());
        // Reserve space.
        learned.reserve(clause.size() + badcl.size());
        // Copy propagating clause, omitting lit.
        std::remove_copy(clause.begin(), clause.end(), learned.end(), lit);
        // Copy conflicting clause, omitting !literal.
        std::remove_copy(badcl.begin(), badcl.end(), learned.end(), lit ^ 1);
        // The learned clause is conflicting.
        badindex = extcnf.size() - 1;
    }
}

bool CDCL_solver::sat()
{
    // # atoms
    Atom atomcount(rcnf.atomcount());
    if (!is1stle2nd(atomcount, max_atom, varfound, varallowed))
        return false;
    // List of decisions
    Decision_list dlist;
    dlist.reserve(atomcount);

    while (1)
    {
        ++count;
        // Read assignment from decision list.
        CNFAssignment assignment(dlist.toassignment(atomcount));
        // Unit propagate.
        std::pair<CNFTruthvalue, CNFClauses::size_type> result
            (extcnf.unitprop(assignment, Unitpropcallback(extcnf, dlist)));
        switch (result.first)
        {
        case CNFTRUE:
            return true; //SATISFIABLE!
        case CNFNONE:
        {
            // clause is UNDECIDED.
            CNFClause const & clause(extcnf[result.second]);
            // lit = NONE.
            Literal lit(clause[CNFclausesat(clause, assignment).second]);
            // Assign lit as a trial decision.
            dlist.push_back(std::make_pair(lit * 2 + 1, 0u));
//std::cout << "Push  " << dlist.back().first << std::endl;
            continue;
        }
        case CNFFALSE: ;
            // clause is CONTRADICTORY.
            if (!dlist.movetonext(learn(result.second, dlist)))
                return false;
        }
    }
}
