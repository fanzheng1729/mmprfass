#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include "base.h"
#include "../cnf.h"
#include "../disjvars.h"
#include "gen.h"

// Propositional proof search, using SAT pruning
struct Prop : SearchBase
{
    Prop(Assiter iter, Database const & database, double const parameters[3]) :
        SearchBase(iter, database, parameters),
        hypscnf(database.propctors().hypscnf(iter->second, hypatomcount))
    {
        Assertion const & ass(iter->second);
        // Relevant syntax axioms
        FOR (Syntaxioms::const_reference syntaxiom, m_database.syntaxioms())
            if (syntaxiom.second < ass)
                syntaxioms.insert(syntaxiom);
    }
    // Check if an assertion is on topic/useful.
    virtual bool ontopic(Assertion const & ass) const
    {
        return ass.type & Assertion::PROPOSITIONAL;
    }
    // Check if a goal is valid.
    virtual bool valid(Proofsteps const & goal) const
    {
        CNFClauses cnf(hypscnf.first);
        Atom natom(hypatomcount);
        if (!m_database.propctors().addclause
            (goal, m_assiter->second.hypiters, cnf, natom))
            return false;
        cnf.closeoff((natom - 1) * 2 + 1);
        return !cnf.sat();
    }
    // Return the hypotheses of a goal to cut.
    virtual Bvector hypstocut(pGoal pgoal) const
    {
        Assertion const & ass(m_assiter->second);
        Bvector result(ass.hypcount(), false);
        Hypsize ntocut(0); // # essential hypothesis to cut
        for (Hypsize i(ass.hypcount() - 1); i != Hypsize(-1); --i)
        {
            if (ass.hypiters[i]->second.second)
                continue; // Skip floating hypothesis.
            result[i] = true;
            CNFClauses const & cnf(m_database.propctors().cnf(ass, pgoal->first,
                                                              result));
            ntocut += result[i] = !cnf.sat();
        }
        return ntocut ? ass.trimvars(result, pgoal->first) : Bvector();
    }
    // Simplify the hypotheses of our node.
    virtual void simphyps(Node const & node) const
    {
        if (node.penv0->addsubenv(node, label(node), assertion(node)))
            static_cast<SearchBase *>(node.penv)->clear();
    }
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address.
    virtual Prop * makeenv(Assiter iter) const
    {
        double const param[3] =
        {exploration()[0], exploration()[1], static_cast<double>(staged)};
        return new(std::nothrow) Prop(iter, m_database, param);
    }
    // Return the simplified assertion for the goal of the node to hold.
    virtual Assertion assertion(Node const & node) const
    {
        Assertion const & oldass(m_assiter->second);
        Bvector const & hypstocut(node.pgoal->second.hypstocut);
        Assertion result;
        result.number = oldass.number;
        result.sethyps(oldass, hypstocut);
        result.expression.resize(1);
        result.disjvars = oldass.disjvars & result.varsused;
        return result;
    }
    virtual ~Prop() {}
private:
    // Add a move with free variables.
    virtual void addhardmove(Assiter iter, Proofsize size, Move & move,
                             Moves & moves) const;
    Syntaxioms syntaxioms;
    Genresult  mutable genresult;
    Termcounts mutable termcounts;
    // The CNF of all hypotheses combined
    Hypscnf const hypscnf;
    Atom hypatomcount;
};

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, Prop::size_type const sizelimit,
     double const parameters[3]);

#endif // PROP_H_INCLUDED
