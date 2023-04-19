#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include <algorithm>
#include "../io.h"
#include "../util/iter.h"
#include "move.h"

// Proof goal
struct Goal
{
    Proofsteps const * prPolish;
    strview typecode;
    Expression expression() const
    {
        Expression result(verifyproofsteps(*prPolish));
        if (!result.empty()) result[0] = typecode;
        return result;
    }
    operator==(Goal other) const
    { return prPolish == other.prPolish && typecode == other.typecode; }
};

// Node in proof search tree
struct Node
{
    typedef ::Move Move;
    typedef ::Moves Moves;
    // Pointer to rev Polish of expression to be proved
    Environ::pGoal pgoal;
    strview typecode;
    Goal goal() const { Goal goal = {&pgoal->first, typecode}; return goal; }
    // Pointer to the parent, if our turn and node is deferred
    Node const * pparent;
    // Proof attempt made, if not our turn
    Move attempt;
    // Essential hypotheses needed, if not our turn
    std::set<Environ::pGoal> hypset;
    // Pointer to the current and initial environments
    Environ *penv, *penv0;
    Node() : pgoal(NULL), typecode(NULL), pparent(NULL), penv(NULL), penv0(NULL) {}
    Node(Environ::pGoal goal, strview type, Environ * p = NULL) :
        pgoal(goal), typecode(type), pparent(NULL), penv(p), penv0(p) {}
    Node(Node const & node) :
        pgoal(node.pgoal), typecode(node.typecode), pparent(&node),
        penv(node.penv), penv0(node.penv0) {}
    // # of defers to the node
    std::size_t defercount() const
    {
        std::size_t count(0);
        for (Node const * p(pparent); p; p = p->pparent)
            ++count;
        return count;
    }
    friend std::ostream & operator<<(std::ostream & out, Node const & node)
    {
        Goal goal = {&node.pgoal->first, node.typecode};
        out << goal.expression();
        if (node.attempt.type != Move::NONE)
            out << "Proof attempt (" << node.defercount() << ") "
                << node.attempt << std::endl;
        return out;
    }
    // Set the hypothesis set of a node.
    void sethyps()
    {
        std::remove_copy_if(attempt.hypvec.begin(), attempt.hypvec.end(),
                            end_inserter(hypset),
                            std::logical_not<const void *>());
    }
    // Check if the hypotheses of node 1 includes those of node 2.
    bool operator>=(Node const & node) const
    {
        return std::includes(hypset.begin(), hypset.end(),
                             node.hypset.begin(), node.hypset.end());
    }
    // Play a move.
    bool play(Move const & move, bool const isourturn)
    {
        if (isourturn)
        {
            // Record the move
            attempt = move;
            return true;
        }
        // Not our turn. Get last move (proof attempt).
        Move const & lastmove(pparent->attempt);
        switch (lastmove.type)
        {
        case Move::ASS:
            // Verify the move.
            if (pgoal->first != lastmove.exprPolish() ||
                typecode != lastmove.exptypecode())
                return false;
            // Pick the hypothesis.
            pgoal = lastmove.hypvec[move.index];
            typecode = lastmove.hyptypecode(move.index);
            pparent = NULL;
            return true;
        case Move::DEFER:
            pparent = pparent->pparent;
//std::cout << "node @ " << this << " in " << penv << ' ' << pparent << '\n';
            return true;
        default: // Empty move
            return false;
        }
    }
    Moves theirmoves() const
    {
        if (attempt.type == Move::ASS)
        {
            Moves result;
            result.reserve(attempt.hypcount() - attempt.varcount());
            for (Moves::size_type i(0); i < attempt.hypcount(); ++i)
                if (!attempt.isfloating(i))
                    result.push_back(i);
            return result;
        }
        return Moves(attempt.type == Move::DEFER, Move::DEFER);
    }
    Moves ourmoves(std::size_t stage) const
    {
        if (penv->done(pgoal, typecode))
            return Moves();
//std::cout << "stage " << stage << std::endl;
        if (penv->staged)
            return penv->ourmoves(*this, stage);

        Moves moves(penv->ourmoves(*this, defercount()));
        moves.push_back(Move::DEFER);
        return moves;
    }
    Moves legalmoves(bool isourturn, std::size_t stage) const
    {
        return isourturn ? ourmoves(stage) : stage > 0 ? Moves() : theirmoves();
    }
    // Add proof for a node using an assertion.
    void writeproof() const
    {
        if (attempt.type != Move::ASS)
            return;
        // Pointers to proofs of hypotheses
        pProofs hyps(attempt.hypcount());
        for (Hypsize i(0); i < attempt.hypcount(); ++i)
        {
            if (attempt.isfloating(i))
                hyps[i] = &attempt.substitutions[attempt.hypiter(i)->second.first[1]];
            else
                hyps[i] = &attempt.hypvec[i]->second.proofsteps;
//std::cout << "Added hyp\n" << *hyps.back();
        }
        // The whose proof
        pgoal->second.status = Environ::PROVEN;
        ::writeproof(pgoal->second.proofsteps, attempt.pass, hyps);
//std::cout << "Written proof\n" << pgoal->first << pgoal->second.proofsteps;
    }
};

#endif // NODE_H_INCLUDED
