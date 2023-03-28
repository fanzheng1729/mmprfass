#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../proof/step.h"
#include "../util/for.h"

// Move in proof search tree
struct Move;
// List of moves
typedef std::vector<Move> Moves;
// Node in proof search tree
struct Node;

inline const void * stepptr(Proofstep step)
{
    if (step.type == Proofstep::HYP)
        return step.phyp;
    else
        return step.pass;
}
inline bool operator<(Proofstep step1, Proofstep step2)
{
    return std::less<const void *>()(stepptr(step1), stepptr(step2));
}

// Proof search environment, containing relevant data
struct Environ
{
    // Evaluation (value, sure?)
    typedef std::pair<double, bool> Eval;
    Environ(Assiter iter) : m_assiter(iter) {}
    // Proof status of a goal
    enum Status {PROVEN = 1, PENDING = 0, FALSE = -1, NEW = -2};
    // Data associated with the goal
    struct Goaldata
    {
        Status status;
        operator Status() const { return status; }
        // Proof of the expression
        Proofsteps proofsteps;
        Goaldata(Status s = PENDING, Proofsteps const & steps = Proofsteps()) :
            status(s), proofsteps(steps) {}
        // If the goal needs hypotheses, i.e., holds conditionally.
        bool hypsneeded;
    };
    // Map: goal -> Evaluation
    typedef std::map<Proofsteps, Goaldata> Goals;
    Goals::pointer addgoal(Proofsteps const & goal, Status s = PENDING)
    { return &*goals.insert(Goals::value_type(goal, s)).first; }
    // Check if an expression is proven or hypothesis.
    // If so, record its proof. Return true iff okay.
    bool done(Goals::pointer pgoal, strview typecode) const;
    // # goals of a given status
    Goals::size_type countgoal(Status status) const
    {
        Goals::size_type n(0);
        FOR (Goals::const_reference goal, goals)
            n += (goal.second == status);
        return n;
    }
    // Move generator, supplied by derived class
    virtual Moves ourlegalmoves(Node const &) const;
    virtual Moves ourlegalmoves(Node const & node, std::size_t stage) const;
    // Evaluation, supplied by derived class
    virtual Eval evalourleaf(Node const &) const = 0;
    virtual Eval evaltheirleaf(Node const &) const = 0;
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address.
    virtual Environ * makeenv(Assiter iter) const = 0;
    // Return the simplified assertion for the goal of the node to hold.
    virtual Assertion assertion(Node const &) const = 0;
    // Add a sub environment for the node. Return true iff it is added.
    bool addsubenv(Node const & node, strview label);
    virtual ~Environ()
    {
        FOR (Subenvs::const_reference subenv, subenvs)
            delete subenv.second;
    }
protected:
    // Iterator to the assertion to be proved
    Assiter m_assiter;
private:
    // Set of goals looked at
    Goals goals;
    // Assertions corresponding to sub environments
    Assertions subassertions;
    // Polymorphic sub environments
    typedef std::map<std::string, Environ *> Subenvs;
    Subenvs subenvs;
};

#endif // ENVIRON_H_INCLUDED
