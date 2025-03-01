#ifndef MCTS1_H_INCLUDED
#define MCTS1_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>
#include "statnode.h"
#include "tree1.h"
#include "../util/arith.h"
#include "../util/for.h"

// Monte-Carlo search tree
template<class Game>
struct MCTS : private Tree<StatNode<Game> >
{
    typedef typename Game::Moves Moves;
    typedef Tree<StatNode<Game> > MCTSTree;
    using typename MCTSTree::size_type;
    using typename MCTSTree::Nodeptr;
    using typename MCTSTree::TreeNoderef;
    using typename MCTSTree::Children;
private:
    static size_type const digits = std::numeric_limits<size_type>::digits;
    // Exploration constant
    double m_exploration[2];
    // Caches for square roots
    double sqrt[2][digits];
//    double sqrt2[digits];
    // Bonus for UCB
//    double m_UCBbonus[2][digits][digits];
    // Initialize the cache.
    void initcache()
    {
        for (int b(0); b < 2; ++b)
            for (size_type i(0); i < digits; ++i)
                sqrt[b][i] = m_exploration[b] * std::sqrt(i);

//        for (size_type i(0); i < digits; ++i)
//            sqrt2[i] = std::sqrt(1 << i);
//
//        for (int b(0); b < 2; ++b)
//            for (size_type i(0); i < digits; ++i)
//                for (size_type j(0); j < digits; ++j)
//                    m_UCBbonus[b][i][j] = sqrt[b][i]/sqrt2[j];

    }
    // # playouts
    size_type m_playcount;
public:
    // Construct a tree with 1 node.
    template<class T>
    MCTS(T const & game, double const exploration[2]) :
        MCTSTree(game), m_playcount(0)
    {
        std::copy(exploration, exploration + 2, m_exploration);
        initcache();
    }
    using MCTSTree::clear;
    using MCTSTree::data;
    using MCTSTree::root;
    using MCTSTree::empty;
    using MCTSTree::size;
    double const * exploration() const { return m_exploration; }
    // Evaluate the root.
    static bool issure(TreeNoderef node) { return node.value().eval().second; }
    bool issure() const { return issure(root()); }
    static bool isourturn(TreeNoderef node) { return node.value().isourturn();}
    static double value(TreeNoderef node) { return node.value().eval().first; }
    double value() const { return value(root()); }
    // Compare 2 nodes, only by their values.
    static bool compvalue(TreeNoderef x, TreeNoderef y)
    { return value(x) < value(y); }
    // Return the UCB bonus term
    double UCBbonus(bool isourturn, size_type parent, size_type self) const
    { return sqrt[isourturn][util::log2(parent)] / std::sqrt(self); }
    // Compute the upper confidence bound.
    double UCB(TreeNoderef node) const
    {
        double const bonus(UCBbonus(!isourturn(node),
                                    node.get().parent->size, node.get().size));
        return value(node) + bonus;
    }
    // Compare 2 children, by UCB and turn.
    // Return -1 if x < y, 0 if x == y, 1 if x > y.
    int compchild(TreeNoderef x, TreeNoderef y) const
    {
        double const dif(UCB(x) - UCB(y));
        int const raw(dif > 0 ? 1 : dif < 0 ? -1: 0);
        return !isourturn(x) ? raw : -raw;
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual double UCBnewstage(TreeNoderef node) const
    {
        static double const max(std::numeric_limits<double>::max());
        return isourturn(node) ? -max : max;
    }
    // Return the unsure child with largest UCB.
    // Return NULL if there is no such a child.
    Nodeptr pickchild(Nodeptr ptr) const
    {
        Children const * const children(ptr.children());
        if (!children) return Nodeptr();
        // Find the first unsure child.
        typedef typename Children::const_iterator Iter;
        Iter child(children->begin());
        for ( ; child != children->end(); ++child)
            if (!issure(*child)) break;
        // If all children are sure, return NULL.
        if (child == children->end())
            return Nodeptr();
        // Find the unsure child with largest UCB.
        for (Iter iter(child); iter != children->end(); ++iter)
        {
            if (issure(*iter)) continue;
            if (compchild(*child, *iter) == -1)
                child = iter;
        }
        // Determine whether to generate a new batch of moves.
//std::cout << isourturn(ptr) << ' ' << UCB(*child) << ' ' << UCBnewstage(ptr) << '\n';
        if (isourturn(ptr) ? UCB(*child) < UCBnewstage(ptr) :
            UCB(*child) > UCBnewstage(ptr))
            return Nodeptr();

        return *child;
    }
    // Move to the unsure child with largest UCB and return true.
    // DO NOTHING and return false if there is no such a child.
    bool pickchild(TreeNoderef & node) const
    {
        Nodeptr child(pickchild(Nodeptr(node)));
        if (child) node = child;
        return child;
    }
    // Return the leaf with largest UCB.
    // Return NULL if ptr is NULL.
    Nodeptr pickleaf(Nodeptr ptr) const
    {
        Nodeptr result;
        while (ptr)
        {
            result = ptr;
            ptr = pickchild(ptr);
        }

        return result;
    }
    Nodeptr pickleaf() const { return pickleaf(data()); }
    // Returns the minimax value of a parent.
    static double parentval(TreeNoderef node)
    {
        Children const & ch(node.get().children);
        return ch.empty() ? 0 :
            value(isourturn(node) ?
                  *std::max_element(ch.begin(), ch.end(), compvalue) :
                  *std::min_element(ch.begin(), ch.end(), compvalue));
    }
    // Evaluate the leaf. Return {value, sure?}.
    // ptr should != NULL.
    virtual Eval evalleaf(TreeNoderef node) const = 0;
    // Evaluate the parent. Return {value, sure?}.
    virtual Eval evalparent(TreeNoderef node) const
    {
        double const value(parentval(node));
        return Eval(value, std::abs(value) == WIN);
    }
    // Evaluate all the new leaves.
    void evalnewleaves(TreeNoderef node) const
    {
        FOR (Nodeptr child, node.get().children)
            if (child.children()->empty())
                child->eval(evalleaf(child));
    }
    // Evaluate the node. Return {value, sure?}.
    // ptr should != NULL.
    Eval evaluate(Nodeptr ptr) const
    { return ptr.children()->empty() ? evalleaf(ptr) : evalparent(ptr); }
    // Call back for back propagation.
    virtual void backpropcallback(TreeNoderef) {}
    // Back propagate from the node pointed.
    // DO NOTHING if ptr is NULL.
    void backprop(Nodeptr ptr)
    {
        for ( ; ptr; ptr = ptr.parent())
        {
//std::cout << "Back prop to " << &*ptr;
            ptr->eval(evaluate(ptr));
//std::cout << "Back prop call back" << std::endl;
            backpropcallback(ptr);
        }
    }
    size_type playcount() const { return m_playcount; }
    // Play out once. Return the value at the root.
    double playonce()
    {
//std::cout << "Playing out ";
        Nodeptr ptr(pickleaf());
//std::cout << "Expanding " << &*ptr << std::endl;
        if (expand<&Game::moves>(ptr))
            evalnewleaves(ptr);
//std::cout << "Back propagating." << std::endl;
        backprop(ptr);
        ++m_playcount;
        return value();
    }
    // Play out until the value is sure or size limit is reached.
    void play(size_type sizelimit)
    {
        if (empty())
            return;
        root().value().eval(evalleaf(root()));
        for ( ; !issure(); playonce())
        {
            if (size() > sizelimit) break;
//std::cout << "value = " << value() << "\tsize = " << size();
//std::cout << (issure() ? "" : " not") << " sure" << std::endl;
//std::cout << "new leaf = " << *pickleaf();
//std::cin.get();
        }
    }
    virtual ~MCTS() {}
private:
    // For all functions below, ptr should != NULL.
    // Add children. Return true iff new children are found.
    bool addchildren(Nodeptr ptr, Moves const & moves)
    {
//std::cout << "Adding " << moves.size() << " nodes to " << *ptr;
        ptr.ref().reserve(moves.size());
        FOR (typename Moves::const_reference move, moves)
        {
            if (!ptr->legal(move))
                continue;
            // Copy node to child.
            Nodeptr child(MCTSTree::insert(ptr, *ptr));
            // Make move to child.
            child->play(move);
        }
//std::cout << ' ' << ptr.children()->size() << " moves added" << std::endl;
        return !ptr.children()->empty();
    }
    // Expand the node pointed. Return true iff new children are found.
    template<Moves (Game::*)(bool) const>
    bool expand(Nodeptr ptr)
    {
        return addchildren(ptr, ptr->moves(isourturn(ptr)));
    }
    template<Moves (Game::*)(bool, stage_t) const>
    bool expand(Nodeptr ptr)
    {
        stage_t & stage(ptr->m_stage);
        return addchildren(ptr, ptr->moves(isourturn(ptr), stage++));
    }
};

#endif // MCTS1_H_INCLUDED
