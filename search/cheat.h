#ifndef CHEAT_H_INCLUDED
#define CHEAT_H_INCLUDED

#include <map>
#include "dummy.h"
#include "../util/for.h"

inline bool ismovehard(Move const & move) { return (void)move, false; }

// Map: expression -> move to prove it.
struct Cheatsheet : std::map<Expression, Move>
{
    // Return the cheat sheet for a proof.
    Cheatsheet(Assiter assiter, Typecodes const & typecodes);
    // Check if a cheat sheet has a step with a hard substitution.
    template<class Pred> bool hashardsubstitution(Pred pred) const
    {
        FOR (const_reference item, *this)
            if (pred(item.second))
                return true;
        return false;
    }
    void print(bool onlyfreesubstitution = false) const;
};

struct Cheatsheet2 : std::map<Proofsteps, Move>
{
    // Return the cheat sheet for a proof.
    Cheatsheet2(Assiter assiter, Typecodes const & typecodes);
};

// Print the cheat sheet of all assertions.
void printcheatsheets(Assiters const & assiters, Typecodes const & typecodes);

// Cheating proof search
struct Cheater : Dummy
{
    Cheater(Assiter iter, Typecodes const & typecodes,
            double const parameters[2]) :
        Dummy(iter, typecodes, parameters), m_cheatsheet(iter, typecodes) {}
    Cheatsheet const & cheatsheet() const { return m_cheatsheet; }
    // UCB threshold for generating a new batch of moves
    // For this value, exploration = 1 works, but = 0 does not work.
    virtual double UCBnewstage(Nodeptr ptr) const
    { static_cast<void>(ptr); return 0.1; }
    virtual Moves ourlegalmoves(Node const & node) const
    {
        Cheatsheet::const_iterator const iter(m_cheatsheet.find(*node.pexp));
//std::cout << iter->second << std::endl;
        return iter == m_cheatsheet.end() ? Moves() : Moves(1, iter->second);
    }
    virtual ~Cheater() {}
private:
    Cheatsheet m_cheatsheet;
};

// Test proof MCTS search. Return 1 iff okay.
bool testproofsearch
    (Assiters const & assiters, Typecodes const & typecodes,
     Cheater::size_type const sizelimit, double const parameters[2],
     Assiters::size_type const batch = 0);

#endif // CHEAT_H_INCLUDED
