#ifndef DISJVARS_H_INCLUDED
#define DISJVARS_H_INCLUDED

#include <iosfwd>
#include "proof.h"

std::ostream & operator<<(std::ostream & out, Disjvars const & disjvars);

// Check if non-dummy variables in two expressions are disjoint.
bool checkdisjvars
    (const Symbol3 * begin1, const Symbol3 * end1,
     const Symbol3 * begin2, const Symbol3 * end2,
     Disjvars const & disjvars, Varsused const * varsused = NULL);
inline bool checkdisjvars
    (Expiter begin1, Expiter end1, Expiter begin2, Expiter end2,
     Disjvars const & disjvars, Varsused const * varsused = NULL)
{
    return checkdisjvars(&*begin1, &*end1, &*begin2, &*end2,
                         disjvars, varsused);
}

#endif // DISJVARS_H_INCLUDED
