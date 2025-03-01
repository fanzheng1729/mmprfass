#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include <algorithm>
#include <cstddef>
#include "../io.h"
#include "../util/iter.h"
#include "move.h"
#include "../MCTS/stageval.h"

// Node in proof search tree
struct Node
{
    typedef ::Move Move;
    typedef ::Moves Moves;
    // Pointer to rev Polish of expression to be proved
    Goalptr goalptr;
    strview typecode;
    Goal goal() const { Goal goal = {goalptr->first, typecode}; return goal; }
    // # defers to the node
    std::size_t defercount;
    // Pointer to the parent, for deferred nodes
    Node const * pparent;
    // Proof attempt made, on their turn
    Move attempt;
    // Essential hypotheses needed, on their turn
    struct Compgoal : std::less<Goalptr>
    {
//        bool operator()(Goalptr p, Goalptr q) const
//        { return p->first < q->first; }
    };
    std::set<Goalptr, Compgoal> hypset;
    // Pointer to the current environment
    Environ *penv;
    Node(Goalptr pgoal = NULL, strview type = NULL, Environ * p = NULL) :
        goalptr(pgoal), typecode(type), defercount(0), penv(p) {}
    Node(Node const & node) :
        goalptr(node.goalptr), typecode(node.typecode), defercount(node.defercount), pparent(&node),
        penv(node.penv) {}
    friend std::ostream & operator<<(std::ostream & out, Node const & node)
    {
        out << node.goal().expression();
        if (node.attempt.type != Move::NONE)
            out << "Proof attempt (" << node.defercount << ") "
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
                             node.hypset.begin(), node.hypset.end(), Compgoal());
    }
    // Check if a move is legal.
    bool legal(Move const & move, bool ourturn) const
    {
        if (ourturn && move.type == Move::ASS) // Check if the goal matches.
            return goalptr->first == move.exprPolish() &&
                    typecode == move.exptypecode();
        if (!ourturn && attempt.type == Move::ASS) // Check index bound.
            return move.index < attempt.hypcount();
        return true;
    }
    // Play a move.
    void play(Move const & move, bool ourturn)
    {
        if (ourturn) // On our turn, record the move.
        {
//std::cout << ' ' << move << " played";
            attempt = move;
            move.type == Move::DEFER ? ++defercount : (defercount = 0);
            return;
        }
        // On their turn, get the last move (proof attempt).
        Move const & lastmove(pparent->attempt);
//std::cout << " parent = " << pparent << ' ' << lastmove << '/' << move.index;
        if (lastmove.type == Move::ASS)
        {
            // Pick the hypothesis.
            goalptr = lastmove.hypvec[move.index];
            typecode = lastmove.hyptypecode(move.index);
        }
    }
    Moves theirmoves() const
    {
//std::cout << "Finding their moves ";
        if (attempt.type == Move::ASS)
        {
            Moves result;
            result.reserve(attempt.hypcount() - attempt.varcount());
//std::cout << "for " << this << ' ' << attempt.pass->first << ": ";
            for (Hypsize i(0); i < attempt.hypcount(); ++i)
                if (!attempt.isfloating(i))
                    result.push_back(i);
//std::cout << result.size() << " moves added" << std::endl;
            return result;
        }
        return Moves(attempt.type == Move::DEFER, Move::DEFER);
    }
    Moves ourmoves(stage_t stage) const
//std::cout << "Finding our moves ";
    {
        if (goalptr->second.status == PROVEN)
            return Moves();
        if (penv->staged)
            return penv->ourmoves(*this, stage);

        Moves moves(penv->ourmoves(*this, defercount));
        moves.push_back(Move::DEFER);
        return moves;
    }
    Moves moves(bool ourturn, stage_t stage) const
    {
        return ourturn ? ourmoves(stage) : stage > 0 ? Moves() : theirmoves();
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
        goalptr->second.status = PROVEN;
        ::writeproof(goalptr->second.proofsteps, attempt.pass, hyps);
//std::cout << penv << " proves " << goal().expression();
//std::cout << goalptr->second.proofsteps;
    }
};

#endif // NODE_H_INCLUDED
