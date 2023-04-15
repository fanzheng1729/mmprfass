#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include "base.h"
#include "../cnf.h"
#include "../database.h"
#include "../disjvars.h"
#include "gen.h"

// Propositional proof search, using SAT pruning
struct Prop : SearchBase
{
    enum { STAGED = 1 };
    Prop(Assiter iter, Database const & database, double const parameters[3]) :
        SearchBase(iter, database.typecodes(), parameters),
        strategy(parameters[2]),
        m_database(database),
        hypscnf(database.propctors().hypscnf(iter->second, hypatomcount))
    {
        Assertion const & ass(iter->second);
        // Relevant syntax axioms
        FOR (Syntaxioms::const_reference syntaxiom, m_database.syntaxioms())
            if (syntaxiom.second < ass)
                syntaxioms.insert(syntaxiom);
    }
    // Size-based score
    static double score(Proofsize size) { return 1. / (size + 1); }
    static Eval eval(Proofsize size) { return Eval(score(size), size == 0); }
    // Check if a goal is valid.
    bool valid(Proofsteps const & goal) const
    {
        CNFClauses cnf(hypscnf);
        Atom natom(hypatomcount);
        if (!m_database.propctors().addcnffromrPolish
            (goal, m_assiter->second.hypiters, cnf, natom))
            throw 1;
        cnf.closeoff(true);
        return !cnf.sat();
    }
    // Check if a goal needs hypotheses, i.e., holds conditionally.
    bool hypsneeded(Proofsteps const & goal) const
    {
        CNFClauses cnf;
        Atom natom(hypatomcount);
        Assertion const & ass(m_assiter->second);
        m_database.propctors().addcnffromrPolish(goal, ass.hypiters, cnf, natom);
        cnf.closeoff(true);
        // counter-satisfiable = needs hypotheses
        return cnf.sat() && ass.esshypcount();
    }
    // Return the extra hypotheses of a goal.
    Bvector extrahyps(Goals::pointer pgoal) const
    {
        Assertion const & ass(m_assiter->second);
        return ass.esshypcount() == 0 || hypsneeded(pgoal->first) ? Bvector() :
            ass.trimvars(Bvector(ass.hypcount(), true), pgoal->first);
    }
    // Evaluate leaf nodes, and record the proof if proven.
    virtual Eval evaltheirleaf(Node const & node) const;
    // Check if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
    virtual Eval evalourleaf(Node const & node) const
    {
//std::cout << "Evaluating " << node;
        if (!node.pgoal->second.extrahyps.empty() && node.penv0->addsubenv(node))
            static_cast<SearchBase *>(node.penv)->clear();
        return eval(node.penv->hypslen
                    + node.pgoal->first.size()
                    + node.defercount());
    }
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address.
    virtual Prop * makeenv(Assiter iter) const
    {
        double const param[3] =
        {exploration()[0], exploration()[1], static_cast<double>(strategy)};
        return new Prop(iter, m_database, param);
    }
    // Return the simplified assertion for the goal of the node to hold.
    virtual Assertion assertion(Node const & node) const
    {
        Assertion const & oldass(m_assiter->second);
        Bvector const & extrahyps(node.pgoal->second.extrahyps);
        Assertion result;
        result.number = oldass.number;
        result.sethyps(oldass, extrahyps);
        result.expression.resize(1);
        result.disjvars = oldass.disjvars & result.varsused;
        return result;
    }
    virtual Eval evalparent(Nodeptr ptr) const
    {
        double const value(parentval(ptr));
        if (ptr->isourturn() && value == -1)
            return evalourleaf(ptr->game());
        return Eval(value, std::abs(value) == 1);
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual double UCBnewstage(Nodeptr ptr) const
    {
        if (!(strategy & STAGED))
            return SearchBase::UCBnewstage(ptr);
        if (!ptr->isourturn())
            return std::numeric_limits<double>::max();
        // Our turn
        Proofsteps const & steps(ptr->game().pgoal->first);
        double const value(score(steps.size() + ptr->stage()));
        return value + UCBbonus(1, ptr.size(), 1);
    }
    // Moves with a given size
    Moves ourmovesbysize(Node const & node, Proofsize size) const;
    virtual Moves ourlegalmoves(Node const & node, std::size_t stage) const
    {
        if (done(node.pgoal, node.typecode))
            return Moves();
//std::cout << "stage " << stage << std::endl;
        if (strategy & STAGED)
            return ourmovesbysize(node, stage);

        Moves moves(ourmovesbysize(node, node.defercount()));
        moves.push_back(Move::DEFER);
        return moves;
    }
    virtual void backpropcallback(Nodeptr ptr)
    {
//std::cout << "Back propagate to " << *ptr;
//std::cin.get();
        // Record the proof for successful attempt.
        if (ptr->eval().first == 1)
            ptr->game().writeproof();
    }
    virtual ~Prop() {}
private:
    // Add a move with no free variables.
    // Return true iff a move closes the goal.
    bool addeasymove(Move const & move, Moves & moves) const
    {
        if (move.closes())
        {
            moves.assign(1, move);
            return true;
        }
        if (valid(move))
            moves.push_back(move);
        return false;
    }
    // Add Hypothesis-oriented moves.
    void addhypmoves(Move const & move, Moves & moves,
                     Subprfsteps const & subprfsteps) const;
    // Add a move with free variables.
    void addhardmove(Assiter iter, Proofsize size, Move & move,
                     Moves & moves) const;
    // Try applying the assertion, and add moves if successful.
    // Return true iff a move closes the goal.
    bool tryassertion
        (Goal goal, Prooftree const & tree, Assiter iter, Proofsize size,
         Moves & moves) const;
    unsigned const strategy;
    Database const & m_database;
    Syntaxioms syntaxioms;
    Genresult  mutable genresult;
    Termcounts mutable termcounts;
    // The CNF of all hypotheses combined
    CNFClauses const hypscnf;
    Atom hypatomcount;
};

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, Prop::size_type const sizelimit,
     double const parameters[3]);

#endif // PROP_H_INCLUDED
