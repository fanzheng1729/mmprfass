#include <iostream>
#include "MCTS.h"

static bool testtree(std::size_t n)
{
    typedef Tree<std::size_t>::Nodeptr Nodeptr;
    // Construct tree 0 -> 1 -> ... -> n.
    Tree<std::size_t> t1(0);
    Nodeptr ptr(t1.data());
    for (std::size_t i(1); i <= n; ++i)
        ptr = t1.insert(ptr, i);
    // Do a breath first search on the tree.
    Tree<std::size_t>::BFSqueue queue(1, t1.data());
    Nodeptr newptr;
    while (1)
    {
        newptr = Tree<std::size_t>::BFSnext(queue);
        if (!newptr) break;
        ptr = newptr;
    }
    // Check.
    if (!ptr && *ptr != n)
        return false;
    // Check copies.
    Tree<std::size_t> t2(t1);
    return t2.size() == n + 1;
}

#include "nimsearch.h"
template<std::size_t N>
static bool checknim(std::size_t const * p, std::size_t sizelimit,
                     double const exploration[2])
{
    NimSearchTree<N> tree(State<Nim<N> >(p), exploration);
    double value(playgame(tree, sizelimit));

    return Nim<N>(p).win() ? value == 1: value == -1;
}

#include "gomsearch.h"
template<std::size_t M, std::size_t N, std::size_t K>
static double playgom(int * p, std::size_t sizelimit,
                      double const exploration[2])
{
    GomSearchTree<M,N,K> tree(State<Gom<M,N,K> >(p), exploration);
    return playgame(tree, sizelimit);
}

// Check Monte Carlo tree search.
bool testMCTS(std::size_t sizelimit, double const exploration[2])
{
    std::cout << "Testing the tree structure." << std::endl;
    if (!testtree(util::log2(sizelimit)))
        return false;

    std::cout << "Playing the game Nim." << std::endl;
    std::size_t v[10] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    if (!checknim<1>(v, sizelimit, exploration))
        return false;
    if (!checknim<9>(v + 1, sizelimit, exploration))
        return false;

    std::cout << "Playing the game Gom." << std::endl;
    if (playgom<2,2,2>(NULL, sizelimit, exploration) != 1)
        return false;
    if (playgom<3,3,3>(NULL, sizelimit, exploration) != 0)
        return false;
    int a[] = {1, 0, -1, -1};
    if (playgom<2,2,2>(a, sizelimit, exploration) != -1)
        return false;

    return true;
}
