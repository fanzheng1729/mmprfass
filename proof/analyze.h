#ifndef ANALYZE_H_INCLUDED
#define ANALYZE_H_INCLUDED

#include "step.h"

// All hypotheses in the proof tree used for a proof step
typedef std::vector<Proofsize> Prooftreehyps;
// Tree structure of the steps of a proof
typedef std::vector<Prooftreehyps> Prooftree;
// Begin and end of a subproof
typedef std::pair<Prfiter, Prfiter> Subprf;
// A sequence of subproofs
typedef std::vector<Subprf> Subprfs;

// Check if a proof has only 1 step using 1 theorem.
inline bool isonestep(Proofsteps const & steps)
{
    if (steps.empty())
        return false;
    // index of the first ASS step
    Proofsize i(0);
    for ( ; i < steps.size() && steps[i].type != Proofstep::ASS; ++i) ;
    // Check if the index is the last one.
    return i == steps.size() - 1;
}

// Return the proof tree. For the step proof[i],
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return empty tree if not okay. Only for uncompressed proofs
Prooftree prooftree(Proofsteps const & steps);

// Return the indentations of all the proofs in a proof tree.
std::vector<Proofsize> indentation(Prooftree const & tree);

// Split a proof for all nodes found in splitters from the root.
// Add to subproofs and return the simplified proof.
Proof splitproof
    (Proof const & proof, Prooftree const & tree,
     // Proof steps to be split.
     Proof const & splitters,
     // (begin, end) of subproofs.
     Subprfs & subproofs);

// Split the assumptions of a theorem.
// Retval[0] = conclusion. Retval[1]... = assumptions.
Subprfs splitassumptions
    (Proof const & proof, Prooftree const & tree,
     // Keyword lists: imps = implications, ands = "and" connectives
     Proof const & imps, Proof const & ands);

// Check if the rPolish of an expression matches a template.
bool findsubstitutions
    (Proofsteps const & exp, Prooftree const & exptree,
     Proofsteps const & pattern, Prooftree const & patterntree,
     Subprfsteps & result);

#endif // ANALYZE_H_INCLUDED
