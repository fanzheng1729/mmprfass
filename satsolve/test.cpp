#include <iostream>
#include "../cnf.h"
#include "../util/arith.h"

static bool checkcnffromtruthtable(Bvector const & tt)
{
    if (tt.empty())
        return false;
    // Check if cnf built from tt has the same truth table as tt.
    Atom const atomcount(util::log2(tt.size()));
    CNFClauses cnf(tt);
    cnf.push_back(CNFClause(1, atomcount * 2));
    if (tt != cnf.truthtable(atomcount))
        return false;
    // Additional test if tt is constant
    bool const isconst(!util::filter(tt)(true) || !util::filter(tt)(false));
    // If it is, it now contains 2 clauses, each with a single literal.
    return !isconst || (cnf.size() == 2 && cnf[0].size() == 1);
}

static const char * checksat(CNFClauses const & cnf, bool const sat)
{
    const char * msg(NULL);

    if (cnf.sat() != sat)
        msg = "cnfsat()";
    else if (cnf.truthtable(0) != Bvector(1, sat))
        msg = "maptruthtable()";
    else if (!checkcnffromtruthtable(cnf.truthtable(cnf.atomcount())))
        msg = "checkcnffromtruthtable()";

    if (msg)
        std::cerr << msg << std::endl;
    return msg;
}

const char * testsat1()
{
    CNFClauses v;
    if (checksat(v, true))
        return "empty instance";
    v.resize(1);
    if (checksat(v, false))
        return "empty clause";
    static const Literal a[4][3] = {
        {0, 2, 5},  // A, B, !C
        {2, 4},     // B, C
        {3},        // !B
        {1, 4}      // !A, C
    };
    v.clear();
    v.push_back(CNFClause(a[0], a[0] + 3));
    v.push_back(CNFClause(a[1], a[1] + 2));
    v.push_back(CNFClause(a[2], a[2] + 1));
    v.push_back(CNFClause(a[3], a[3] + 2));
    if (checksat(v, true))
        return "satisfiable instance";
    v[3][1] = 5;    // !A, !C
    if (checksat(v, false))
        return "satisfiable instance";

    return "OKay";
}

// Test maximal SAT instances from 1 atom up to n atoms.
// Return 0 if okay; otherwise return # atoms in wrong CNF.
unsigned testsat2(unsigned n)
{
    n = std::min<unsigned>(n,std::numeric_limits<CNFClauses::size_type>::digits);

    for (unsigned i(1); i <= n; ++i)
    {
        // Create a CNF with all possible 2^i clauses.
        CNFClauses cnf(1u << i);
        for (CNFClauses::size_type j(0); j < cnf.size(); ++j)
        {
            cnf[j].resize(i);
            for (CNFClause::size_type k(0); k < i; ++k)
                cnf[j][k] = (j >> k) & 1;
        }
        // This CNF should be UNSATISFIABLE.
        if (cnf.sat()) return i;
        // Remove the last clause.
        cnf.pop_back();
        // This CNF should be SATISFIABLE.
        if (!cnf.sat()) return i;
    }

    return 0;
}
