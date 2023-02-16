#include <algorithm>
#include <utility>
#include "io.h"
#include "proof.h"
#include "util/filter.h"
#include "util/for.h"
#include "util/iter.h"

std::ostream & operator<<(std::ostream & out, Disjvars const & disjvars)
{
    FOR (Disjvars::const_reference r, disjvars)
        out << r.first.c_str << ' ' << r.second.c_str << '\n';

    return out;
}

// Check if two variables satisfy the disjoint variable hypothesis.
static bool checkdisjvars
    (strview var1, strview var2, Disjvars const & disjvars)
{
    if (var1 == var2) return false;
    if (disjvars.count(std::make_pair(var1, var2)) > 0) return true;
    if (disjvars.count(std::make_pair(var2, var1)) > 0) return true;
    return false;
}

// Check if two sets satisfy the disjoint variable hypothesis.
static bool checkdisjvars
    (Symbol3s const & set1, Symbol3s const & set2,
     Disjvars const & disjvars)
{
    FOR (Symbol3 var1, set1)
        FOR (Symbol3 var2, set2)
            if (!checkdisjvars(var1, var2, disjvars))
            {
                std::cerr << var1 << " and " << var2
                          << " violate the disjoint variable hypothesis\n";
                return false;
            }

    return true;
}

// Compare a symbol and an extended symbol.
static bool operator<(Symbol3 var1, Varsused::const_reference var2)
{
    return var1 < var2.first;
}
static bool operator<(Varsused::const_reference var1, Symbol3 var2)
{
    return var1.first < var2;
}

// Check if non-dummy variables in two expressions are disjoint.
bool checkdisjvars
    (const Symbol3 * begin1, const Symbol3 * end1,
     const Symbol3 * begin2, const Symbol3 * end2,
     Disjvars const & disjvars, Varsused const * varsused = NULL)
{
    // Variables in the two expressions
    Symbol3s set1, set2;
    std::remove_copy_if(begin1, end1, end_inserter(set1), isconst);
    std::remove_copy_if(begin2, end2, end_inserter(set2), isconst);

    if (!is_disjoint(set1.begin(), set1.end(), set2.begin(), set2.end()))
    {
        std::cerr << "Expression\n" << Expression(begin1, end1) << "and"
                  << "Expression\n" << Expression(begin2, end2) << "have"
                  << "have a common variable" << std::endl;
        return false;
    }
    // Check disjoint variable hypotheses on used variables.
    return checkdisjvars(varsused ? set1 & *varsused : set1,
                         varsused ? set2 & *varsused : set2, disjvars);
}
