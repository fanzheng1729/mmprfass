#ifndef GOMSEARCH_H_INCLUDED
#define GOMSEARCH_H_INCLUDED

#include <algorithm>
#include "MCTS.h"
#include "gom.h"
#include "../util.h"
#include "../util/for.h"

// Search tree for the game
template<std::size_t M, std::size_t N, std::size_t K>
struct GomSearchTree : MCTS<Gom<M,N,K> >
{
    typedef MCTS<Gom<M,N,K> > MCTSearch;
    using typename MCTSearch::Nodeptr;
    GomSearchTree
        (State<Gom<M,N,K> > const & state, double const exploration[2]) :
            MCTSearch(state, exploration) {}
    // Evaluate the leaf. Return {value, sure?}
    // ptr should != NULL.
    virtual Eval evalleaf(Nodeptr ptr) const
    {
        Gom<M,N,K> const & game(ptr->game());
        int const winner(game.winner());
        return Eval(winner, winner || game.full());
    }
    // Evaluate the parent. Return {value, sure?}
    // ptr should != NULL.
    virtual Eval evalparent(Nodeptr ptr) const
    {
        double const value(this->parentval(ptr));
        // If a parent has value +/-1 then it is sure.
        if (std::abs(value) == 1)
            return Eval(value, true);
        // Otherwise it is sure only if all its children are sure.
        bool sure(true);
        FOR (Nodeptr child, *ptr.children())
            if (!this->issure(child))
                sure = false;

        return Eval(value, sure);
    }
};

#endif // GOMSEARCH_H_INCLUDED
