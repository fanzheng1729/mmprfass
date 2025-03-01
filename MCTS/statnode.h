#ifndef STATNODE_H_INCLUDED
#define STATNODE_H_INCLUDED

#include <iostream>
#include "stageval.h"

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
    using Game::moves;
    bool legal(Move const & move) const { return Game::legal(move,m_ourturn); }
    void play(Move const & move)
    {
        Game::play(move, m_ourturn);
        m_ourturn ^= true;
    }
    friend std::ostream & operator<<(std::ostream & out, State const & state)
    {
        static char const s[2][10] = {"Thr turn ", "Our turn "};
        out << s[state.isourturn()] << state.game();
        return out;
    }
};

// Monto-Carlo search tree
template<class Game>
struct MCTS;
template<class Game>
struct MCTS2;

// Status node base = {game state, evaluation}
template<class Game>
class StatNodeBase : public State<Game>
{
    // Evaluation, from -1 to 1
    double value;
    // Is evaluation sure?
    bool sure;
    // Update evaluation.
    void eval(Eval v) { value = v.first, sure = v.second; }
    friend MCTS<Game>;
    friend MCTS2<Game>;
public:
    template<class T>
    StatNodeBase(T const & game) :
        State<Game>(game), value(0), sure(false) {}
    Eval eval() const { return Eval(value, sure); }
    friend std::ostream & operator<<(std::ostream & out, StatNodeBase const & node)
    {
        out << "Eval: " << node.eval().first << &"*\t"[1 - node.sure]
            << static_cast<State<Game> const &>(node) << std::endl;
        return out;
    }
};

// Check if a game supports staged move generation
template<class Game>
class Isstaged
{
    typedef char One;
    typedef char Two[2];
    static One & check(typename Game::Moves (Game::*)(bool) const);
    static Two & check(typename Game::Moves (Game::*)(bool, stage_t) const);
public:
    static const bool value = sizeof(check(Game::moves)) - 1;
};

// Stage count, if Game supports staged move generation
template<class Game, bool>
class MCTStageBase;
template<class Game>
class MCTStageBase<Game, true>
{
    // Stage of move generation
    stage_t m_stage;
    friend MCTS<Game>;
    friend MCTS2<Game>;
public:
    MCTStageBase() : m_stage(0) {}
    MCTStageBase(MCTStageBase const &) : m_stage(0) {}
    stage_t stage() const { return m_stage; }
};
template<class Game>
class MCTStageBase<Game, false>
{
public:
    stage_t stage() const { return 0; }
};
template<class Game>
struct MCTStage : MCTStageBase<Game, Isstaged<Game>::value> {};

// Status node = {node base, Stage}
template<class Game>
struct StatNode : StatNodeBase<Game>, MCTStage<Game>
{
    template<class T>
    StatNode(T const & game) : StatNodeBase<Game>(game), MCTStage<Game>() {}
};

#endif // STATNODE_H_INCLUDED
