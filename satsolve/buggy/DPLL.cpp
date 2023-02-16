#include "DPLL.h"
#include "../../io.h"
#include "../../msg.h"
#include "../../util/for.h"

CNFAssignment DPLL_solver::Decision_list::toassignment(Atom atomcount) const
{
    CNFAssignment assignment(atomcount);
    FOR (value_type decision, *this)
        assignment.assign(decision / 2);

    return assignment;
}

bool DPLL_solver::Decision_list::movetonext()
{
    // Find the last trial decision.
    reverse_iterator const iter(std::find_if(rbegin(), rend(), istrial));
//std::cout << "Erase " << iter.base() - begin() << std::endl;
    // If there is no trial decision, the CNF is UNSATISFIABLE.
    if (iter == rend()) return false;
    // Remove all decisions after it.
    erase(iter.base(), end());
    // Flip the trial and make it non-trial.
    back() ^= 3;
//std::cout << "Push  " << back() << std::endl;
    return true;
}

bool DPLL_solver::sat()
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
            (rcnf.unitprop(assignment, Unitpropcallback(rcnf, dlist)));
        switch (result.first)
        {
        case CNFTRUE:
            return true; // SATISFIABLE!
        case CNFNONE:
        {
            // clause is UNDECIDED.
            CNFClause const & clause(rcnf[result.second]);
            // lit = NONE.
            Literal lit(clause[CNFclausesat(clause, assignment).second]);
            // Assign lit as a trial decision.
            dlist.push_back(lit * 2 + 1);
//std::cout << "Push  " << dlist.back() << std::endl;
            continue;
        }
        case CNFFALSE:
            // clause is CONTRADICTORY.
            if (!dlist.movetonext())
                return false;
        }
    }
}
