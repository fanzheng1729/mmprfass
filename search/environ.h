#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../database.h"
#include "../util/for.h"
#include "goal.h"
#include "../MCTS/stageval.h"

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

// Size-based score
inline double score(Proofsize size) { return 1. / (size + 1); }
inline Eval eval(Proofsize size) { return Eval(score(size), size == 0); }

// Proof search environment, containing relevant data
struct Environ
{
    Environ(Assertion const & ass, Database const & db, bool isstaged = 0) :
        m_database(db), staged(isstaged), hypslen(ass.hypslen()), m_ass(ass),
        m_number(ass.number), penv0(this) {}
    Environ(Assertion const & ass, Database const & db,
            Assertions::size_type number, bool isstaged = 0) :
        m_database(db), staged(isstaged), hypslen(ass.hypslen()), m_ass(ass),
        m_number(number), penv0(this) {}
    // Map: name -> polymorphic sub environments
    typedef std::map<std::string, Environ *> Subenvs;
    // Add a goal. Return its pointer.
    Goalptr addgoal(Proofsteps const & goal, Goalstatus s = PENDING)
    { return &*goals.insert(Goals::value_type(goal, s)).first; }
    // Check if an expression is proven or hypothesis.
    // If so, record its proof. Return true iff okay.
    bool done(Goalptr goalptr, strview typecode) const
    {
        if (goalptr->second.status == PROVEN)
            return true; // already proven

        Hypsize const i(m_ass.matchhyp(goalptr->first, typecode));
        if (i == m_ass.hypcount())
            return false; // Hypothesis matched
        // Write the 1-step proof.
        goalptr->second.status = PROVEN;
        goalptr->second.proofsteps.assign(1, m_ass.hypiters[i]);
        return true;
    }
    // # goals of a given status
    Goals::size_type countgoal(int status) const
    {
        Goals::size_type n(0);
        FOR (Goals::const_reference goal, goals)
            n += (goal.second.status == status);
        FOR (Subenvs::const_reference subenv, subenvs)
            n += subenv.second->countgoal(status);
        return n;
    }
    // # sub environments
    Subenvs::size_type countenvs() const { return subenvs.size() + 1; }
    // Check if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return ass.number; }
    // Return the hypotheses of a goal to be trimmed.
    virtual Bvector hypstotrim(Goalptr p) const { return Bvector(0 && p); }
    // Check if a goal is valid.
    virtual bool valid(Proofsteps const & goal) const { return !goal.empty(); }
    // Check if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
    // Moves generated at a given stage
    virtual Moves ourmoves(Node const & node, stage_t stage) const;
    // Evaluate leaf nodes, and record the proof if proven.
    virtual Eval evalourleaf(Node const & node) const;
    virtual Eval evaltheirleaf(Node const & node) const;
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeenv(Assertion const &) const { return NULL; };
    // Return the simplified assertion for the goal of the node to hold.
    virtual Assertion makeass(Node const &) const { return Assertion(); }
    // Add a sub environment for the node. Return true iff it is added.
    bool addsubenv(Node const & node);
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
    // The assertion to be proved
    Assertion const & m_ass;
private:
    // Only use assertions before this #.
    Assertions::size_type const m_number;
    // Pointer to the root environment
    Environ * penv0;
    // Set of goals looked at
    Goals goals;
    // Assertions corresponding to sub environments
    Assertions subassertions;
    // Polymorphic sub environments
    Subenvs subenvs;
    // Add a move with only bound substitutions.
    // Return true if it has no essential hypotheses.
    bool addboundmove(Move const & move, Moves & moves) const;
    // Add Hypothesis-oriented moves. Return false.
    bool addhypmoves(Move const & move, Moves & moves,
                     Subprfsteps const & subprfsteps) const;
    // Add a move with free variables. Return false.
    virtual bool addhardmoves(Assiter iter, Proofsize size, Move & move,
                             Moves & moves) const
                             { return &move == &moves[size] && !&*iter; }
    // Try applying the assertion, and add moves if successful.
    // Return true iff a move closes the goal.
    bool tryassertion
        (Goal goal, Prooftree const & tree, Assiter iter, Proofsize size,
         Moves & moves) const;
};

#endif // ENVIRON_H_INCLUDED
