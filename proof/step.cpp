#include <iostream>
#include "../ass.h"
#include "../scope.h"
#include "step.h"
#include "../util/for.h"

const char proofsteperr[] = "Invalid proof step ";

Proofstep::Proofstep(strview label, Assertions const & assertions,
                     struct Scopes const & scopes)
{
    // Check if token names an assertion.
    Assiter iterass(assertions.find(label));
    *this = iterass != assertions.end() ? Proofstep(&*iterass) :
        Proofstep(scopes.activehypptr(label));
}

// Return the name of the proof step.
Proofstep::operator const char*() const
{
    switch (type)
    {
    case HYP:
        return phyp->first.c_str;
    case ASS:
        return pass->first.c_str;
    default:
        std::cerr << proofsteperr << "of type " << type << std::endl;
    }
    return NULL;
}

// label -> proof step, using hypotheses and assertions.
Proofstep Proofstep::Builder::operator()(strview label) const
{
    Hypiter const iterhyp(m_hyps.find(label));
    if (iterhyp != m_hyps.end())
        return Proofstep(iterhyp); // hypothesis

    Assiter const iterass(m_assertions.find(label));
    if (iterass != m_assertions.end())
        return Proofstep(&*iterass); // assertion

    std::cerr << proofsteperr << label.c_str << std::endl;
    return Proofstep::NONE;
}

// Write the proof from pointers to proof of hypotheses.
void writeproof(Proofsteps & dest, Assptr pass, pProofs const & hyps)
{
    // Total length ( +1 for label)
    Proofsize length(1);
    FOR (Proofsteps const * p, hyps)
        length += p->size();

    // Hypotheses
    dest.clear();
    dest.reserve(length); // Preallocate for efficiency
    FOR (Proofsteps const * p, hyps)
        dest += *p;
    // Label of the assertion used
    dest.push_back(pass);
    //std::cout << "Built proof: " << proof;
}
