#ifndef MCTS2_H_INCLUDED
#define MCTS2_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>
#include "statnode.h"
#include "tree2.h"
#include "../util/arith.h"
#include "../util/for.h"

// Monte-Carlo search tree
template<class Game>
struct MCTS2 : private Tree2<StatNode<Game> >
{
    typedef typename Game::Moves Moves;
    typedef Tree2<StatNode<Game> > MCTSTree2;
    using typename MCTSTree2::size_type;
    using typename MCTSTree2::TreeNoderef;
    using typename MCTSTree2::Children;
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
    MCTS2(T const & game, double const exploration[2]) :
        MCTSTree2(game), m_playcount(0)
    {
        std::copy(exploration, exploration + 2, m_exploration);
        initcache();
    }
    using MCTSTree2::clear;
    using MCTSTree2::data;
    using MCTSTree2::root;
    using MCTSTree2::empty;
    using MCTSTree2::size;
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
    // Move to the unsure child with largest UCB and return true.
    // DO NOTHING and return false if there is no such a child.
    bool pickchild(TreeNoderef & node) const
    {
        Children const & children(node.get().children);
        if (children.empty())
            return false;
        // Find the first unsure child.
        typedef typename Children::const_iterator Iter;
        Iter child(children.begin());
        for ( ; child != children.end(); ++child)
            if (!issure(*child)) break;
        // If all children are sure, return false.
        if (child == children.end())
            return false;
        // Find the unsure child with largest UCB.
        for (Iter iter(child); iter != children.end(); ++iter)
        {
            if (issure(*iter)) continue;
            if (compchild(*child, *iter) == -1)
                child = iter;
        }
        // Determine whether to generate a new batch of moves.
//std::cout << isourturn(node) << ' ' << UCB(*child) << ' ' << UCBnewstage(node) << '\n';
        if (isourturn(node) ? UCB(*child) < UCBnewstage(node) :
            UCB(*child) > UCBnewstage(node))
            return false;

        node = *child;
        return true;
    }
    // Move to the leaf with largest UCB.
    void pickleaf(TreeNoderef & node) const { while (pickchild(node)) {} }
    TreeNoderef pickleaf()
    {
        TreeNoderef leaf(root());
        pickleaf(leaf);
        return leaf;
    }
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
        FOR (TreeNoderef child, node.get().children)
            if (child.get().children.empty())
                child.value().eval(evalleaf(child));
    }
    // Evaluate the node. Return {value, sure?}.
    Eval evaluate(TreeNoderef node) const
    { return node.get().children.empty() ? evalleaf(node) : evalparent(node); }
    // Call back for back propagation.
    virtual void backpropcallback(TreeNoderef) {}
    // Back propagate from the node pointed.
    void backprop(TreeNoderef node)
    {
        while (true)
        {
//std::cout << "Back prop to " << &node.value();
            node.value().eval(evaluate(node));
//std::cout << "Back prop call back" << std::endl;
            backpropcallback(node);
            if (typename MCTSTree2::TreeNode * p = node.get().parent)
                node = *p;
            else
                break;
        }
    }
    size_type playcount() const { return m_playcount; }
    // Play out once. Return the value at the root.
    double playonce()
    {
//std::cout << "Playing out ";
        TreeNoderef node(pickleaf());
//std::cout << "Expanding " << &node.value() << std::endl;
        if (expand<&Game::moves>(node))
            evalnewleaves(node);
//std::cout << "Back propagating." << std::endl;
        backprop(node);
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
//std::cout << "new leaf = " << pickleaf().value();
//std::cin.get();
        }
    }
    virtual ~MCTS2() {}
private:
    // Add children. Return true iff new children are found.
    bool addchildren(TreeNoderef node, Moves const & moves)
    {
//std::cout << "Adding " << moves.size() << " nodes to " << &node.value();
        node.reserve(moves.size());
        FOR (typename Moves::const_reference move, moves)
        {
            if (!node.value().legal(move))
                continue;
            // Copy node to child.
            TreeNoderef child(node.insert(node.value()));
            // Make move to child.
            child.value().play(move);
        }
//std::cout << ' ' << node.get().children.size() << " moves added" << std::endl;
        return !node.get().children.empty();
    }
    // Expand the node pointed. Return true iff new children are found.
    template<Moves (Game::*)(bool) const>
    bool expand(TreeNoderef node)
    {
        return addchildren(node, node.value().moves(isourturn(node)));
    }
    template<Moves (Game::*)(bool, stage_t) const>
    bool expand(TreeNoderef node)
    {
        stage_t & stage(node.value().m_stage);
        return addchildren(node, node.value().moves(isourturn(node), stage++));
    }
};

#endif // MCTS2_H_INCLUDED
