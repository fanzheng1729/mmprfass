#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include "environ.h"
#include "../MCTS/MCTS.h"
#include "node.h"

// Proof search tree base class, implementing loop detection
struct SearchBase : Environ, MCTS2<Node>
{
    enum { STAGED = 1 };
    SearchBase(Assertion const & ass, Database const & db,
               double const params[3], Assertions::size_type number = 0) :
        Environ(ass, db, number ? number : ass.number,
                static_cast<unsigned>(params[2]) & STAGED),
        MCTS2(Node(), params)
    {
        if (ass.expression.empty()) return;
        Goalptr goalptr(addgoal(ass.exprPolish));
        // The node is copied when constructing the search tree,
        // making the pointer to parent dangling, so it must be reset.
        const_cast<Node &>(root().value().game()) =
        Node(goalptr, ass.expression[0], this);
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual double UCBnewstage(TreeNoderef treenode) const
    {
        if (!(staged & STAGED))
            return MCTS2<Node>::UCBnewstage(treenode);
        if (!isourturn(treenode))
            return std::numeric_limits<double>::max();
        // Our turn
        Node const & node(treenode.value().game());
        double const value(score(node.penv->hypslen
                                 + node.goalptr->first.size()
                                 + treenode.value().stage()));
        return value + UCBbonus(1, treenode.get().size, 1);
    }
    virtual Eval evalleaf(TreeNoderef treenode) const
    {
        Node const & node(treenode.value().game());
        bool loopsback(SearchBase::TreeNoderef);
        return loopsback(treenode) ? Eval(LOSS, true) :
            isourturn(treenode) && done(node.goalptr, node.typecode) ? Eval(WIN, true) :
                isourturn(treenode) ? node.penv->evalourleaf(node) :
                    node.penv->evaltheirleaf(node);
    }
    virtual Eval evalparent(TreeNoderef treenode) const
    {
        Node const & node(treenode.value().game());
        double const value(parentval(treenode));
        if (staged && isourturn(treenode) && value == LOSS)
            return node.penv->evalourleaf(node); // Current stage no valid attempt
        return Eval(value, std::abs(value) == WIN);
    }
    // Record the proof of proven goals on back propagation.
    virtual void backpropcallback(TreeNoderef node)
    {
        if (value(node) == WIN)
            node.value().game().writeproof();
    }
    // Proof of the assertion, if any
    Proofsteps const & proof() const
    {
        return root().value().game().goalptr->second.proofsteps;
    }
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(TreeNoderef node, bool detailed = false) const;
    void printmainline(bool detailed = false) const
    { printmainline(root(), detailed); }
    void printstats() const;
    void navigate(bool detailed = true) const;
    virtual ~SearchBase() {}
};

#endif // BASE_H_INCLUDED
