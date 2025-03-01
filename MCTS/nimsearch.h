#ifndef NIMSEARCH_H_INCLUDED
#define NIMSEARCH_H_INCLUDED

#include "MCTS.h"
#include "nim.h"

// Search tree for the game
template<template<class> class MCTS, std::size_t N>
struct NimSearchTree : MCTS<Nim<N> >
{
    typedef MCTS<Nim<N> > MCTSearch;
    using typename MCTSearch::TreeNoderef;
    NimSearchTree
        (State<Nim<N> > const & state, double const exploration[2]) :
            MCTSearch(state, exploration) {}
    // Evaluate the leaf. Return {value, sure?}
    // ptr should != NULL.
    virtual Eval evalleaf(TreeNoderef node) const
    {
        State<Nim<N> > const & state(node.value());
        return state.game().sum() == 0 ? // Game over?
            Eval(state.isourturn() ? -1 : 1, true) :
            Eval(0, false);
    }
};

#endif // NIMSEARCH_H_INCLUDED
