#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include <cstddef>
#include <memory>
#include "../proof/step.h"
#include "../util/for.h"

// Move in proof search tree
struct Move;
// List of moves
typedef std::vector<Move> Moves;
// Node in proof search tree
struct Node;

// Proof search environment, containing relevant data
struct Environ
{
    // Evaluation (value, sure?)
    typedef std::pair<double, bool> Eval;
    Environ(Assiter iter) : m_assiter(iter) {}
    enum Value {PROVEN = 1, PENDING = 0, FALSE = -1, NEW = -2};
    // Data associated with the goal
    struct Goaldata
    {
        Value value;
        operator Value() const { return value; }
        // Proof of the expression
        Proofsteps proofsteps;
        Goaldata(Value v = PENDING, Proofsteps const & steps = Proofsteps()) :
            value(v), proofsteps(steps) {}
        // If the goal needs hypotheses, i.e., holds conditionally.
        bool hypsneeded;
    };
    // Map: goal -> Evaluation
    typedef std::map<Proofsteps, Goaldata> Goals;
    Goals::pointer addgoal(Proofsteps const & goal, Value value = PENDING)
    { return &*goals.insert(std::make_pair(goal, value)).first; }
    // Check if an expression is proven or hypothesis.
    // If so, record its proof. Return true iff okay.
    bool done(Goals::pointer pgoal, strview typecode) const;
    // Add proof for a node using an assertion.
    void writeproof(Node const & node) const;
    // Proof of the assertion, if any
    Proofsteps proof() const;
    // # goals proven
    Goals::size_type countgoal(Value type) const
    {
        Goals::size_type n(0);
        FOR (Goals::const_reference goal, goals)
            n += (goal.second == type);
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
    // Add a sub environment for the node. Return true iff it is added.
    bool addenviron(Node & node, strview label, Assertion const & newass);
    virtual ~Environ()
    {
        FOR (Subenvs::value_type subenv, subenvs)
            delete subenv.second;
    }
protected:
    // Iterator to the assertion to be proved
    Assiter m_assiter;
    // Assertions corresponding to sub environments
    Assertions subassertions;
    // Polymorphic sub environments
    typedef std::map<std::string, Environ *> Subenvs;
    Subenvs subenvs;
private:
    // Set of goals looked at
    Goals goals;
};

#endif // ENVIRON_H_INCLUDED
