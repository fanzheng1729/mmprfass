#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include "environ.h"
#include "../MCTS/MCTS.h"
#include "node.h"

// Proof search tree base class, implementing loop detection
struct SearchBase : Environ, MCTS<Node>
{
    typedef ::Node Node;
    enum { STAGED = 1 };
    SearchBase(Assiter iter, Database const & db, double const parameters[3]) :
        Environ(iter, db, static_cast<unsigned>(parameters[2]) & STAGED),
        MCTS(Node(), parameters)
    {
        if (iter->second.expression.empty())
            return;
        Goalptr goalptr(Environ::addgoal(iter->second.exprPolish));
        // The node is copied when constructing the search tree,
        // making the pointer to parent dangling, so it must be reset.
        const_cast<Node &>(data()->game()) =
        Node(goalptr, iter->second.expression[0], this);
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual double UCBnewstage(Nodeptr ptr) const
    {
        if (!(staged & STAGED))
            return MCTS<Node>::UCBnewstage(ptr);
        if (!ptr->isourturn())
            return std::numeric_limits<double>::max();
        // Our turn
        Node const & node(ptr->game());
        double const value(score(node.penv->hypslen
                                 + node.goalptr->first.size()
                                 + ptr->stage()));
        return value + UCBbonus(1, ptr.size(), 1);
    }
    virtual Eval evalleaf(Nodeptr ptr) const
    {
        Node const & node(ptr->game());
        bool loopsback(SearchBase::Nodeptr ptr);
        return loopsback(ptr) ? Eval(-1, true) :
            ptr->isourturn() && done(node.goalptr, node.typecode) ? Eval(1, true) :
                ptr->isourturn() ? node.penv->evalourleaf(node) :
                    node.penv->evaltheirleaf(node);
    }
    virtual Eval evalparent(Nodeptr ptr) const
    {
        double const value(parentval(ptr));
        if (staged && ptr->isourturn() && value == -1)
            return ptr->game().penv->evalourleaf(ptr->game()); // Current stage no valid attempt
        return Eval(value, std::abs(value) == 1);
    }
    // Record the proof of proven goals on back propagation.
    virtual void backpropcallback(Nodeptr ptr)
    {
//std::cout << "Back propagate to " << *ptr;
//std::cin.get();
        if (ptr->eval().first == 1)
            ptr->game().writeproof();
    }
    // Proof of the assertion, if any
    Proofsteps const & proof() const
    {
        return data()->game().goalptr->second.proofsteps;
    }
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(Nodeptr ptr, bool detailed = false) const;
    void printmainline(bool detailed = false) const
    { printmainline(data(), detailed); }
    void printstats() const;
    void navigate(bool detailed = true) const;
    virtual ~SearchBase() {}
};

#endif // BASE_H_INCLUDED
