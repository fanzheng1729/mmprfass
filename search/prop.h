#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include "base.h"
#include "../cnf.h"
#include "../database.h"
#include "gen.h"

// Propositional proof search, using SAT pruning
struct Prop : SearchBase
{
    enum { STAGED = 1 };
    Prop(Assiter iter, Database const & database, double const parameters[3]) :
        SearchBase(iter, database.typecodes(), parameters),
        strategy(parameters[2]),
        m_database(database),
        varsused(iter->second.varsused),
        hypcnf(database.propctors().hypcnf(iter->second, hypatomcount)),
        hypslen(iter->second.hypslen())
    {
        Assertion const & ass(iter->second);
        // Assume that all hypotheses are necessary for the root goal.
        data()->game().pgoal->second.hypsneeded = ass.hypcount() > ass.varcount();
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
        CNFClauses cnf(hypcnf);
        Atom natom(hypatomcount);
        if (!m_database.propctors().addcnffromrPolish
            (goal, m_assiter->second.hypiters, cnf, natom))
            return false;
        cnf.closeoff(true);
        return !cnf.sat();
    }
    // Check if a goal needs hypotheses, i.e., holds conditionally.
    bool hypsneeded(Proofsteps const & goal) const
    {
        CNFClauses cnf;
        Atom natom(hypatomcount);
        m_database.propctors().addcnffromrPolish
            (goal, m_assiter->second.hypiters, cnf, natom);
        cnf.closeoff(true);
        return cnf.sat(); // counter-satisfiable = needs hypotheses.
    }
    // Evaluate leaf nodes, and record the proof if proven.
    virtual Eval evaltheirleaf(Node const & node) const;
    virtual Eval evalourleaf(Node const & node) const
    {
        bool const okay(done(node.pgoal, node.typecode));
        if (okay)
            return Eval(1, true);
//std::cout << "Evaluating " << node;
        const_cast<Prop &>(*this).addsubenv(const_cast<Node &>(node));
        Proofsize newhypslen(static_cast<Prop *>(node.penv)->hypslen);
        return eval(newhypslen + node.pgoal->first.size() + node.defercount());
    }
    // Returns the label of a sub environment from a node.
    std::string subenvlabel(Node const &) const { return ""; }
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address.
    virtual Prop * makeenv(Assiter iter) const
    {
        double const param[3] =
        {exploration()[0], exploration()[1], static_cast<double>(strategy)};
        return new Prop(iter, m_database, param);
    }
    Assertion subassertion(Node const &) const
    {
        Assertion assertion;
        Assertion const & oldassertion(m_assiter->second);
        assertion.number = oldassertion.number;
        assertion.expression.resize(1);
        for (Hypsize i(0); i < oldassertion.hypcount(); ++i)
        {
            Hypiter iter(oldassertion.hypiters[i]);
            if (!iter->second.second)
                continue; // Skip essential hypotheses.
            assertion.hypiters.push_back(iter);
            assertion.hypsrPolish.push_back(oldassertion.hypsrPolish[i]);
            assertion.hypstree.push_back(oldassertion.hypstree[i]);
        }
        assertion.varsused = oldassertion.varsused;
        assertion.disjvars = oldassertion.disjvars;
        return assertion;
    }
    // Add a sub environment for the node. Return true iff it is added.
    void addsubenv(Node & node)
    {
        Assertion const & oldass(m_assiter->second);
        if (oldass.hypcount()==oldass.varcount() || node.pgoal->second.hypsneeded)
            return;
//std::cout << "Unconditional goal: " << node.pgoal->first;
        std::string const & label(subenvlabel(node));
        Assertion const & newass(subenvs.count(label) ? oldass:
                                 subassertion(node));
        if (Environ::addsubenv(node, label, newass))
            static_cast<Prop *>(node.penv)->clear();
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
    // Check if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
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
        Node const & node(ptr->game());
        if (ptr->eval().first != 1 || node.attempt.type != Move::ASS)
            return;
        // Record the proof for successful attempt.
        writeproof(node);
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
    Varsused varsused;
    Syntaxioms syntaxioms;
    Genresult  mutable genresult;
    Termcounts mutable termcounts;
    CNFClauses const hypcnf;
    Atom hypatomcount;
    // length of the rev Polish notation of all hypotheses combined
    Proofsize  const hypslen;
};

// Test propositional proof search. Return 1 iff okay.
Prop::size_type testpropsearch
    (Assiter iter, Database const & database, Prop::size_type sizelimit,
     double const parameters[3]);
bool testpropsearch
    (Database const & database, Prop::size_type const sizelimit,
     double const parameters[3]);

#endif // PROP_H_INCLUDED
