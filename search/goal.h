#ifndef GOAL_H_INCLUDED
#define GOAL_H_INCLUDED

#include "../proof/verify.h"

// Proof goal
struct Goal
{
    Proofsteps const & prPolish;
    strview typecode;
    Expression expression() const
    {
        Expression result(verifyproofsteps(prPolish));
        if (!result.empty()) result[0] = typecode;
        return result;
    }
    operator==(Goal other) const
    { return &prPolish == &other.prPolish && typecode == other.typecode; }
};

// Proof status of a goal
enum Goalstatus {PROVEN = 1, PENDING = 0, FALSE = -1, NEW = -2};

// Data associated with the goal
struct Goaldata
{
    Goalstatus status;
    // Proof of the expression
    Proofsteps proofsteps;
    Goaldata(Goalstatus s = PENDING, Proofsteps const & steps = Proofsteps()) :
        status(s), proofsteps(steps) {}
    // Unnecessary hypothesis of the goal
    Bvector hypstotrim;
};
// Map: goal -> Evaluation
typedef std::map<Proofsteps, Goaldata> Goals;
// Pointer to a goal
typedef Goals::pointer Goalptr;

#endif // GOAL_H_INCLUDED
