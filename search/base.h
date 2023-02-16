#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include "../MCTS/MCTS.h"
#include "../comment.h"
#include "node.h"
#include "../util/filter.h"
#include "../util/iter.h"

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
    // Check if goal appears as the goal of ptr or its ancestors.
    static bool makesloop(Goal goal, Nodeptr ptr)
    {
        if (!ptr->isourturn())
            ptr = ptr.parent();
        for ( ; ptr; ptr = ptr.parent().parent())
        {
            Node const & node(ptr->game());
            if (!node.pparent && goal == node.goal())
                return true;
        }
        return false;
    }
    // Check if ptr is harder than any children of ancestor.
    static bool makesloop(Nodeptr ptr, Nodeptr ancestor)
    {
        Node const & node(ptr->game());
        FOR (Nodeptr child, *ancestor.children())
        {
            Node const & sibling(child->game());
            if (sibling.attempt.type != Move::ASS || child->eval().first == -1)
                continue;
            if (child.parent() == ptr.parent() &&
                !sibling.hypvec.empty() && sibling.hypset.empty())
                continue; // brother not checked
            if (&node != &sibling && node >= sibling)
                return true;
        }
        return false;
    }
    static makesloop(Nodeptr ptr)
    {
        Node const & node(ptr->game());
        if (node.attempt.type != Move::ASS)
            return false;
        // Now it is a move using an assertion.
        // Check if any of the hypotheses appears in a parent node.
        sethyps(const_cast<Node &>(ptr->game()));
        for (Hypsize i(0); i < node.attempt.hypcount(); ++i)
        {
            if (node.attempt.isfloating(i))
                continue; // Skip floating hypotheses.
            Goal goal = {&node.hypvec[i]->first, node.attempt.hyptypecode(i)};
            if (makesloop(goal, ptr.parent()))
                return true;
        }
        // Check if this node is harder than a parent node.
        Nodeptr ancestor(ptr.parent());
        while (true)
        {
            if (makesloop(ptr, ancestor))
                return true;
            // Move up.
            if (!ancestor.parent())
                return false;
            ancestor = ancestor.parent().parent();
        }
        return false;
    }
    virtual Eval evalleaf(Nodeptr ptr) const
    {
        Node const & node(ptr->game());
        return makesloop(ptr) ? Eval(-1, true) :
            ptr->isourturn() ? node.penv->evalourleaf(node) :
                node.penv->evaltheirleaf(node);
    }
    // Printing utilities. DO NOTHING if ptr is NULL.
    void printtheirnode(Nodeptr ptr) const;
    void printourchildren(Nodeptr ptr) const;
    void printournode(Nodeptr ptr, bool detailed = true) const;
    void printmainline(Nodeptr ptr, bool detailed = true) const;
    void printmainline(bool detailed = true) const
    { printmainline(data(), detailed); }
    void printfulltree(Nodeptr ptr, size_type level = 0) const;
    void printfulltree() const;
    void printstats() const;
    void navigate(bool detailed = true) const;
    void help() const;
    virtual ~SearchBase() {}
    Typecodes const & typecodes;
protected:
    Nodeptr findourchild(Nodeptr ptr, strview ass, size_type index) const;
    // Set the hypothesis set of a node.
    static void sethyps(Node & node)
    {
        std::remove_copy_if(node.hypvec.begin(), node.hypvec.end(),
                            end_inserter(node.hypset),
                            std::logical_not<const void *>());
    }
};

#endif // BASE_H_INCLUDED
