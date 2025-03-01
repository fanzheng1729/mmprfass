#include <iostream>
#include "MCTS.h"
#include "../util/timer.h"

static bool testtree(std::size_t n)
{
    typedef Tree<std::size_t> Testtree;
    typedef Tree2<std::size_t> Testtree2;
std::cout << "Making tree 0 -> 1 -> ... -> " << n << std::endl;
    Testtree t1(chain1(n));
#if __cplusplus >= 201103L
    Testtree2 t3(chain2(n));
#else
    Testtree t3(chain1(n));
#endif // __cplusplus
std::cout << "Checking the tree " << std::endl;
    if (!t1.check() || !t3.check())
        return false;
std::cout << "Copying the tree" << std::endl;
    Testtree t2(t1);
    Testtree2 t4(t3);
std::cout << "Checking the tree " << std::endl;
    if (!t2.check() || !t4.check())
        return false;
    return t2.size() == n + 1 && t4.size() == n + 1;
}

// Play out until the value is sure or size limit is reached.
// Return the value.
template<class Tree>
static double playgame(Tree & tree, typename Tree::size_type sizelimit)
{
    Timer timer;
    tree.play(sizelimit);
    // Collect statistics.
    double const t(timer);
    if (tree.size() > sizelimit)
        std::cerr << "Tree size limit exceeded. ";
    std::cout << tree.size() << " nodes / " << t << "s = ";
    std::cout << tree.size()/t << " nps" << std::endl;
    return tree.value();
}

#include "nimsearch.h"
template<std::size_t N>
static bool checknim(std::size_t const * p, std::size_t sizelimit,
                     double const exploration[2])
{
    NimSearchTree<MCTS, N> tree(State<Nim<N> >(p), exploration);
    NimSearchTree<MCTS2,N> tree2(State<Nim<N> >(p),exploration);
    double const value(playgame(tree, sizelimit));
    double const value2(playgame(tree2, sizelimit));
    if (value != value2) return false;
    return Nim<N>(p).win() ? value == WIN: value == LOSS;
}

#include "gomsearch.h"
template<std::size_t M, std::size_t N, std::size_t K>
static double playgom(int * p, std::size_t sizelimit,
                      double const exploration[2])
{
    GomSearchTree<MCTS, M,N,K> tree(State<Gom<M,N,K> >(p), exploration);
    GomSearchTree<MCTS2,M,N,K> tree2(State<Gom<M,N,K> >(p),exploration);
    return playgame(tree, sizelimit), playgame(tree2, sizelimit);
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
    if (playgom<2,2,2>(NULL, sizelimit, exploration) != WIN)
        return false;
    if (playgom<3,3,3>(NULL, sizelimit, exploration) != DRAW)
        return false;
    int a[] = {0, 0, 0, -1};
    if (playgom<2,2,2>(a, sizelimit, exploration) != LOSS)
        return false;

    return true;
}
