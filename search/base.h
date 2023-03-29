#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include "../MCTS/MCTS.h"
#include "node.h"
#include "../typecode.h"
#include "../util/filter.h"

// Proof search tree base class, implementing loop detection
struct SearchBase : Environ, MCTS<Node>
{
    typedef ::Node Node;
    typedef Environ::Eval Eval;
    SearchBase(Assiter iter, Typecodes const & types, double const parameters[2]) :
        Environ(iter), MCTS(Node(), parameters), typecodes(types)
    {
        // The search works with rPolish, not expression.
        Goals::pointer pgoal(Environ::addgoal(iter->second.exprPolish));
        // The node is copied when constructing the search tree,
        // making the pointer to parent dangling, so it must be reset.
        const_cast<Node &>(data()->game()) =
        Node(pgoal, iter->second.expression[0], this);
    }
    virtual Eval evalleaf(Nodeptr ptr) const
    {
        Node const & node(ptr->game());
        bool makesloop(SearchBase::Nodeptr ptr);
        return makesloop(ptr) ? Eval(-1, true) :
            ptr->isourturn() ? node.penv->evalourleaf(node) :
                node.penv->evaltheirleaf(node);
    }
    // Proof of the assertion, if any
    Proofsteps const & proof() const
    {
        return data()->game().pgoal->second.proofsteps;
    }
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(Nodeptr ptr, bool detailed = true) const;
    void printmainline(bool detailed = true) const
    { printmainline(data(), detailed); }
    void printfulltree() const;
    void printstats() const;
    void navigate(bool detailed = true) const;
    static void help();
    virtual ~SearchBase() {}
    Typecodes const & typecodes;
};

#endif // BASE_H_INCLUDED
