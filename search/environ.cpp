#include "environ.h"
#include "io.h"
#include "move.h"
#include "node.h"

// Move generator, supplied by derived class
Moves Environ::ourlegalmoves(Node const &) const { return Moves(); }
Moves Environ::ourlegalmoves(Node const & node, std::size_t stage) const
{
    return stage > 0 ? Moves() : ourlegalmoves(node);
}

Proofsteps Environ::proof() const
{
    return goals.at(m_assiter->second.exprPolish).proofsteps;
}

// Check if an expression is proven or hypothesis.
// If so, record its proof. Return true iff okay.
bool Environ::done(Goals::pointer pgoal, strview typecode) const
{
    if (pgoal->second.value == PROVEN)
        return true;

    Assertion const & ass(m_assiter->second);
    Hypsize const i(ass.matchhyp(pgoal->first, typecode));
    if (i == ass.hypcount())
        return false;

    pgoal->second.value = PROVEN;
    pgoal->second.proofsteps.assign(1, ass.hypiters[i]);
    return true;
}

// Add proof for a node using an assertion.
void Environ::writeproof(Node const & node) const
{
    Move const & move(node.attempt);
    // Pointers to proofs of hypotheses
    pProofs hyps;
    // Preallocate for efficiency.
    hyps.reserve(move.hypcount());
    for (Hypsize i(0); i < move.hypcount(); ++i)
    {
        if (move.isfloating(i))
        {
            // Floating hypothesis
            Hypiter iter(move.pass->second.hypiters[i]);
            hyps.push_back(&move.substitutions[iter->second.first[1]]);
        }
        else
        {
            // Essential hypothesis
            hyps.push_back(&node.hypvec[i]->second.proofsteps);
        }
//std::cout << "Added hyp\n" << *hyps.back();
    }
    // The whose proof
    node.pgoal->second.value = PROVEN;
    ::writeproof(node.pgoal->second.proofsteps, move.pass, hyps);
//std::cout << "Written proof\n" << node.pgoal->first << node.pgoal->second.proofsteps;
}

// Add a sub environment for the node.
bool Environ::addenviron(Node & node, strview label, Assertion const & ass)
{
    Subenvs::iterator iter(subenvs.find(label));
    if (iter != subenvs.end())
    {
        // Sub environment already exists.
        // Point the node's environment to it.
        node.penv = iter->second;
        return false;
    }
//std::cout << "Adding sub environment for " << node.pgoal->first;
    std::pair<strview, Environ *> value(label, NULL);
    iter = subenvs.insert(value).first;
    // Add the simplified assertion.
    Assiter assiter
        (subassertions.insert(std::make_pair(iter->first, ass)).first);
    // Pointer to the sub environment
    Environ * penv(makeenv(assiter));
    iter->second = penv;
    // Prepare the sub environment.
    penv->m_assiter = assiter;
    // Point the node's environment to the sub environment.
    node.penv = penv;
    return true;
}
