#include <limits>
#include "io.h"
#include "msg.h"

#include "satsolve/DPLL.h"
typedef DPLL_solver Solver_used;

// Return true if the SAT instance is satisfiable.
bool CNFClauses::sat() const
{
    return empty() || Solver_used(*this).sat();
}

// Map: free atoms -> truth value.
// Return the empty vector if not okay.
Bvector Satsolver::truthtable(Atom const nfree)
{
    static Atom const maxnfree(std::numeric_limits<Atom>::digits);
    if (!is1stle2nd(nfree, std::min(maxnfree, rcnf.atomcount()),
                    varfound, varallowed))
        return Bvector();

    Bvector truthtable(1u << nfree, false);
    if (rcnf.hasemptyclause())
        return truthtable;

    CNFClauses cnf2(rcnf);
    for (Bvector::size_type arg(0); arg < (1u << nfree); ++arg)
    {
        for (Atom i(0); i < nfree; ++i)
            cnf2.push_back(CNFClause(1, i * 2 + 1 - (arg >> i & 1)));
        truthtable[arg] = cnf2.sat();
        cnf2.resize(rcnf.size());
    }

    return truthtable;
}

// Map: free atoms -> truth value.
// Return the empty vector if not okay.
Bvector CNFClauses::truthtable(Atom const nfree) const
{
    return Solver_used(*this).truthtable(nfree);
}
