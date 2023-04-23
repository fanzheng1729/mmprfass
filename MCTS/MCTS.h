#ifndef MCTS_H_INCLUDED
#define MCTS_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>
#include "../util/timer.h"
#include "tree.h"
#include "../util/arith.h"
#include "../util/for.h"

// Monto-Carlo search tree
template<class Game>
struct MCTS;

// Game state = {game, bool = is our turn?}
template<class Game>
class State : Game
{
    bool m_ourturn;
public:
    using typename Game::Move;
    template<class T>
    State(T const & game, bool isourturn = true) :
        Game(game), m_ourturn(isourturn) {}
    Game const & game() const { return *this; }
    bool isourturn() const { return m_ourturn; }
    using Game::legalmoves;
    bool play(Move const & move)
    {
        bool const okay(Game::play(move, m_ourturn));
        m_ourturn ^= okay;
        return okay;
    }
    friend std::ostream & operator<<(std::ostream & out, State const & state)
    {
        static char const s[2][10] = {"Thr turn", "Our turn "};
        out << s[state.isourturn()] << state.game();
        return out;
    }
};

// Evaluation (value, sure?)
typedef std::pair<double, bool> Eval;

// Node of the search tree = {game state, evaluation}
template<class Game>
class MCTSNode : public State<Game>
{
    // Evaluation, from -1 to 1
    double value;
    // Is evaluation sure?
    bool sure;
    // Update evaluation.
    void eval(Eval v) { value = v.first, sure = v.second; }
    // Stage of move generation
    typename Tree<MCTSNode<Game> >::size_type m_stage;
    friend MCTS<Game>;
public:
    template<class T>
    MCTSNode(T const & game) :
        State<Game>(game), value(0), sure(false), m_stage(0) {}
    Eval eval() const { return Eval(value, sure); }
    typename Tree<MCTSNode<Game> >::size_type stage() const { return m_stage; }
    friend std::ostream & operator<<(std::ostream & out, MCTSNode const & node)
    {
        out << "Eval: " << node.eval().first << &"*\t"[1 - node.sure]
            << static_cast<State<Game> const &>(node) << std::endl;
        return out;
    }
};

// Monte-Carlo search tree
template<class Game>
struct MCTS : private Tree<MCTSNode<Game> >
{
    typedef typename Game::Moves Moves;
    typedef Tree<MCTSNode<Game> > MCTSTree;
    using typename MCTSTree::size_type;
    using typename MCTSTree::Nodeptr;
    using typename MCTSTree::Children;
    typedef typename Children::const_iterator Nodeptriter;
private:
    static size_type const digits = std::numeric_limits<size_type>::digits;
    // Exploration constant
    double m_exploration[2];
    // Caches for square roots
    double sqrt[2][digits];//, sqrt2[digits];
    // Bonus for UCB
//    double m_UCBbonus[2][digits][digits];
    // Initialize the cache.
    void initcache()
    {
        for (int b(0); b < 2; ++b)
            for (size_type i(0); i < digits; ++i)
                sqrt[b][i] = m_exploration[b] * std::sqrt(i);
/*
        for (size_type i(0); i < digits; ++i)
            sqrt2[i] = std::sqrt(1 << i);

        for (int b(0); b < 2; ++b)
            for (size_type i(0); i < digits; ++i)
                for (size_type j(0); j < digits; ++j)
                    m_UCBbonus[b][i][j] = sqrt[b][i]/sqrt2[j];
*/
    }
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
    using MCTSTree::empty;
    using MCTSTree::size;
    double const * exploration() const { return m_exploration; }
    // Evaluate the root.
    static issure(Nodeptr ptr) { return ptr->eval().second; }
    bool issure() const { return issure(data()); }
    double value() const { return data()->eval().first; }
    void reeval(Nodeptr ptr, Eval eval) { ptr->eval(eval); }
    // Compare 2 nodes, only by their values.
    static bool compvalue(Nodeptr x, Nodeptr y)
    { return x->eval().first < y->eval().first; }
    // Return the UCB bonus term
    double UCBbonus(bool isourturn, size_type parent, size_type self) const
    { return sqrt[isourturn][util::log2(parent)]/std::sqrt(self); }
    // Compute the upper confidence bound.
    // Return 0 if ptr is NULL.
    double UCB(Nodeptr ptr) const
    {
        if (!ptr) return 0;
        double const value(ptr->eval().first);
        return value+UCBbonus(!ptr->isourturn(),ptr.parent().size(),ptr.size());
    }
    // Compare 2 children, by UCB and turn.
    // Return -1 if x < y, 0 if x == y, 1 if x > y.
    int compchild(Nodeptr x, Nodeptr y) const
    {
        double const dif(UCB(x) - UCB(y));
        int const raw(dif > 0 ? 1 : dif < 0 ? -1: 0);
        return !x->isourturn() ? raw : -raw;
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual double UCBnewstage(Nodeptr ptr) const
    { return (1 - 2 * ptr->isourturn()) * std::numeric_limits<double>::max(); }
    // Return the unsure child with largest UCB.
    // Return NULL if there is no such a child.
    Nodeptr pickchild(Nodeptr ptr) const
    {
        Children const * const children(ptr.children());
        if (!children || children->empty())
            return Nodeptr();
        // Find the first unsure child.
        Nodeptriter child(children->begin());
        for ( ; child != children->end(); ++child)
            if (!(*child)->eval().second)
                break;
        // If all children are sure, return NULL.
        if (child == children->end())
            return Nodeptr();
        // Find the unsure child with largest UCB.
        for (Nodeptriter iter(child); iter != children->end(); ++iter)
        {
            if ((*iter)->eval().second)
                continue;
            if (compchild(*child, *iter) == -1)
                child = iter;
        }
        // Determine whether to generate a new batch of moves.
//std::cout << ptr->isourturn() << ' ' << UCB(*child) << ' ' << UCBnewstage(ptr) << '\n';
        if (ptr->isourturn() ? UCB(*child) < UCBnewstage(ptr) :
            UCB(*child) > UCBnewstage(ptr))
            return Nodeptr();

        return *child;
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
    // Expand the node pointed. Return true iff new children are found.
    // ptr should != NULL.
    bool expand(Nodeptr ptr) { return doexpand(ptr, &Game::legalmoves); }
    // Returns the minimax value of a parent.
    // ptr should != NULL
    static double parentval(Nodeptr ptr)
    {
        Children const * pc(ptr.children());

        Nodeptr child;
        if (ptr->isourturn())
            child = *std::max_element(pc->begin(), pc->end(), compvalue);
        else
            child = *std::min_element(pc->begin(), pc->end(), compvalue);

        return child->eval().first;
    }
    // Evaluate the leaf. Return {value, sure?}.
    // ptr should != NULL.
    virtual Eval evalleaf(Nodeptr ptr) const = 0;
    // Evaluate the parent. Return {value, sure?}.
    // ptr should != NULL.
    virtual Eval evalparent(Nodeptr ptr) const
    {
        double const value(parentval(ptr));
        return Eval(value, std::abs(value) == 1);
    }
    // Evaluate all the new leaves.
    // ptr should != NULL.
    void evalnewleaves(Nodeptr ptr) const
    {
        FOR (Nodeptr child, *ptr.children())
            if (child && child.children()->empty())
                child->eval(evalleaf(child));
    }
    // Evaluate the node. Return {value, sure?}.
    // ptr should != NULL.
    Eval evaluate(Nodeptr ptr) const
    {
        return ptr.children()->empty() ? evalleaf(ptr) : evalparent(ptr);
    }
    // Call back for back propagation.
    // ptr should != NULL.
    virtual void backpropcallback(Nodeptr)
    {
//std::cout << "Back prop to " << *ptr;
    }
    // Back propagate from the node pointed.
    // DO NOTHING if ptr is NULL.
    void backprop(Nodeptr ptr)
    {
        for ( ; ptr; ptr = ptr.parent())
        {
            ptr->eval(evaluate(ptr));
            backpropcallback(ptr);
        }
    }
    // Play out once. Return the value at the root.
    double playonce()
    {
        if (playcount() == 0)
        {
            // First play
            if (data()->eval().second) // Finished
                return data()->eval().first;
            data()->eval(evalleaf(data()));
            if (data()->eval().second) // Finished
                return data()->eval().first;
        }

        Nodeptr ptr(pickleaf(data()));
//std::cout << "new leaf = " << *ptr;
        if (expand(ptr))
            evalnewleaves(ptr);
//std::cout << "Back propagating." << std::endl;
        backprop(ptr);
        ++m_playcount;
        return value();
    }
    // Play out until the value is sure or size limit is reached.
    void play(size_type sizelimit)
    {
        if (!data())
            return;
        for ( ; !issure(); playonce())
        {
            if (size() > sizelimit) break;
//std::cout << "value = " << value() << "\tsize = " << size() << std::endl;
//std::cout << "new leaf = " << *pickleaf(data());
//std::cin.get();
        }
    }
    size_type playcount() const { return m_playcount; }
    virtual ~MCTS() {}
private:
    // # playouts
    size_type m_playcount;
    // Add children. Return true iff new children are found.
    // ptr should != NULL.
    bool addchildren(Nodeptr ptr, Moves const & moves)
    {
//std::cout << "Adding " << moves.size() << " nodes to " << *ptr;
        FOR (typename Moves::const_reference move, moves)
        {
            // Copy node to child.
            Nodeptr child(MCTSTree::insert(ptr, *ptr));
            // Make move to child.
            if (child->play(move))
                child->m_stage = 0; // Child has not been expanded.
            else // Illegal move
                MCTSTree::clearchild(ptr, ptr.children()->size() - 1);
        }
        return !ptr.children()->empty();
    }
    // Expand the node pointed. Return true iff new children are found.
    // ptr should != NULL.
    bool doexpand(Nodeptr ptr, Moves (Game::*)(bool) const)
    {
//std::cout << "finding legal moves for " << *ptr;
        return ptr->m_stage++ > 0 ? false :
            addchildren(ptr, ptr->legalmoves(ptr->isourturn()));
    }
    template<class T>
    bool doexpand(Nodeptr ptr, Moves (Game::*)(bool, T) const)
    {
//std::cout << "finding legal moves for " << *ptr;
        return addchildren(ptr, ptr->legalmoves(ptr->isourturn(),
                                                ptr->m_stage++));
    }
};

// Play out until the value is sure or size limit is reached.
// Return the value.
template<class Game>
double playgame(MCTS<Game> & tree, typename MCTS<Game>::size_type sizelimit)
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

#endif // MCTS_H_INCLUDED
