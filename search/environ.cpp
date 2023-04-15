#include "environ.h"
#include "move.h"
#include "node.h"

Environ::Environ(Assiter iter) :
    hypslen(iter->second.hypslen()), m_assiter(iter) {}

// Move generator, supplied by derived class
Moves Environ::ourlegalmoves(Node const &) const { return Moves(); }
Moves Environ::ourlegalmoves(Node const & node, std::size_t stage) const
{
    return stage > 0 ? Moves() : ourlegalmoves(node);
}

// Check if a goal is proven or matches a hypothesis.
// If so, record its proof. Return true iff okay.
bool Environ::done(Goals::pointer pgoal, strview typecode) const
{
    if (pgoal->second.status == PROVEN)
        return true; // already proven
    // Check if the goal matches a hypothesis.
    Assertion const & ass(m_assiter->second);
    Hypsize const i(ass.matchhyp(pgoal->first, typecode));
    if (i == ass.hypcount())
        return false;
    // The whose proof (1 step)
    pgoal->second.status = PROVEN;
    pgoal->second.proofsteps.assign(1, ass.hypiters[i]);
    return true;
}

// Returns the label of a sub environment from a node.
std::string Environ::label(Node const & node, char separator) const
{
    return m_assiter->second.hypslabel(node.pgoal->second.extrahyps, separator);
}

// Add a sub environment for the node. Return true iff it is added.
bool Environ::addsubenv(Node const & node, strview label)
{
    // Node's sub environment pointer
    Environ * & penv(const_cast<Environ * &>(node.penv));
    // Find if the sub environment named label already exists.
    Subenvs::iterator enviter(subenvs.find(label));
    if (enviter != subenvs.end())
    {
        // Yes. Point the node's sub environment pointer to it.
        penv = enviter->second;
        return false;
    }
//std::cout << "Adding sub environment " << label << " for " << node.pgoal->first;
//std::cin.get();
    enviter = subenvs.insert(std::pair<strview, Environ *>(label, NULL)).first;
    // Simplified assertion
    Assertions::value_type const ass(enviter->first, assertion(node));
    // Iterator to the simplified assertion
    Assiter const assiter(subassertions.insert(ass).first);
    // Point the node's environment to the sub environment.
    penv = enviter->second = makeenv(assiter);
    // Prepare the sub environment.
    penv->m_assiter = assiter;
    return true;
}
