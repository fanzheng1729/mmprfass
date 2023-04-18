#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../database.h"
#include "../proof/step.h"
#include "../util/for.h"

// Move in proof search tree
struct Move;
// List of moves
typedef std::vector<Move> Moves;
// Node in proof search tree
struct Node;
// Proof goal
struct Goal;

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

// Evaluation (value, sure?)
typedef std::pair<double, bool> Eval;
// Size-based score
inline double score(Proofsize size) { return 1. / (size + 1); }
inline Eval eval(Proofsize size) { return Eval(score(size), size == 0); }

// Proof search environment, containing relevant data
struct Environ
{
    Environ(Assiter iter, Database const & database, bool isstaged = false) :
        m_database(database),
        staged(isstaged), hypslen(iter->second.hypslen()), m_assiter(iter) {}
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
        // Unnecessary hypothesis of the goal
        Bvector extrahyps;
    };
    // Map: goal -> Evaluation
    typedef std::map<Proofsteps, Goaldata> Goals;
    Goals::pointer addgoal(Proofsteps const & goal, Status s = PENDING)
    { return &*goals.insert(Goals::value_type(goal, s)).first; }
    // Check if an expression is proven or hypothesis.
    // If so, record its proof. Return true iff okay.
    bool done(Goals::pointer pgoal, strview typecode) const;
    // # goals of a given status
    Goals::size_type countgoal(int status) const
    {
        Goals::size_type n(0);
        FOR (Goals::const_reference goal, goals)
            n += (goal.second == status);
        return n;
    }
    // Check if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return &ass == &ass; }
    // Return the extra hypotheses of a goal.
    virtual Bvector extrahyps(Goals::pointer pgoal) const
    { return Bvector(pgoal - pgoal); }
        // Check if a goal is valid.
    virtual bool valid(Proofsteps const & goal) const { return &goal==&goal; }
    // Check if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
    // Moves generated at a given stage
    virtual Moves ourmoves(Node const & node, std::size_t stage) const;
    // Evaluate leaf nodes, and record the proof if proven.
    virtual Eval evalourleaf(Node const &) const;
    virtual Eval evaltheirleaf(Node const & node) const;
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeenv(Assiter) const {return NULL; };
    // Return the simplified assertion for the goal of the node to hold.
    virtual Assertion assertion(Node const &) const { return Assertion(); };
    // Returns the label of a sub environment from a node.
    std::string label(Node const & node, char separator = '+') const;
    // Add a sub environment for the node. Return true iff it is added.
    bool addsubenv(Node const & node, strview label);
    bool addsubenv(Node const & node) { return addsubenv(node, label(node)); }
    virtual ~Environ()
    {
        FOR (Subenvs::const_reference subenv, subenvs)
            delete subenv.second;
    }
    // Database to be used
    Database const & m_database;
    // Is staged move generation turned on?
    bool const staged;
    // Length of the rev Polish notation of all hypotheses combined
    Proofsize const hypslen;
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
    // Add a move with no free variables.
    // Return true iff a move closes the goal.
    bool addeasymove(Move const & move, Moves & moves) const;
    // Add Hypothesis-oriented moves.
    void addhypmoves(Move const & move, Moves & moves,
                     Subprfsteps const & subprfsteps) const;
    // Add a move with free variables.
    virtual void addhardmove(Assiter iter, Proofsize size, Move & move,
                             Moves & moves) const
                             { &move == &moves[size] ? *iter : *iter; }
    // Try applying the assertion, and add moves if successful.
    // Return true iff a move closes the goal.
    bool tryassertion
        (Goal goal, Prooftree const & tree, Assiter iter, Proofsize size,
         Moves & moves) const;
};

#endif // ENVIRON_H_INCLUDED
