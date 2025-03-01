#ifndef GOMSEARCH_H_INCLUDED
#define GOMSEARCH_H_INCLUDED

#include <algorithm>
#include "MCTS.h"
#include "gom.h"
#include "../util.h"
#include "../util/for.h"

// Search tree for the game
template<template<class> class MCTS, std::size_t M, std::size_t N, std::size_t K>
struct GomSearchTree : MCTS<Gom<M,N,K> >
{
    typedef MCTS<Gom<M,N,K> > MCTSearch;
    using typename MCTSearch::TreeNoderef;
    GomSearchTree
        (State<Gom<M,N,K> > const & state, double const exploration[2]) :
            MCTSearch(state, exploration) {}
    // Evaluate the leaf. Return {value, sure?}
    // ptr should != NULL.
    virtual Eval evalleaf(TreeNoderef node) const
    {
        Gom<M,N,K> const & game(node.value().game());
        int const winner(game.winner());
        return Eval(winner, winner || game.full());
    }
    // Evaluate the parent. Return {value, sure?}
    // ptr should != NULL.
    virtual Eval evalparent(TreeNoderef node) const
    {
        double const value(this->parentval(node));
        // If a parent has value +/-1 then it is sure.
        if (std::abs(value) == 1)
            return Eval(value, true);
        // Otherwise it is sure only if all its children are sure.
        bool sure(true);
        FOR (TreeNoderef child, node.get().children)
            if (!this->issure(child))
                sure = false;

        return Eval(value, sure);
    }
};

#endif // GOMSEARCH_H_INCLUDED
