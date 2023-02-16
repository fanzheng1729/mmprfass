#ifndef STEP_H_INCLUDED
#define STEP_H_INCLUDED

#include "../proof.h"

// Step in a regular or compressed proof
struct Proofstep
{
    typedef std::vector<Expression>::size_type Index;
    enum Type
    {
        NONE, HYP, ASS, LOAD, SAVE
    } type;
    union
    {
        Hypptr phyp;
        Assptr pass;
        Index index;
    };
    Proofstep(Type t = NONE) : type(t == SAVE ? t : NONE) {}
    Proofstep(Hypiter iter) : type(HYP), phyp(&*iter) {}
//    Proofstep(Assiter iter) : type(ASS), pass(&*iter) {}
    Proofstep(Hypptr p) : type(p ? HYP : NONE), phyp(p) {}
    Proofstep(Assptr p) : type(p ? ASS : NONE), pass(p) {}
    Proofstep(Index i) : type(LOAD), index(i) {}
    Proofstep(strview label, Assertions const & assertions,
              struct Scopes const & scopes);
// Return the name of the proof step.
    operator const char *() const;
    Symbol2::ID id() const
    {
        return type == HYP && phyp->second.second ? phyp->second.first[1] : 0;
    }
    struct Builder
    {
        Builder(Hypotheses const & hyps, Assertions const & assertions)
                : m_hyps(hyps), m_assertions(assertions) {}
// label -> proof step, using hypotheses and assertions.
        Proofstep operator()(strview label) const;
    private:
        Hypotheses const & m_hyps;
        Assertions const & m_assertions;
    };
};

// A sequence of proof steps
typedef std::vector<Proofstep> Proofsteps;
// # of step in the proof
typedef Proofsteps::size_type Proofsize;
// Iterator to a proof step
typedef Proofsteps::const_iterator Stepiter;
// Begin and end of a subsequence of steps
typedef std::pair<Stepiter, Stepiter> Subprfstep;
// A subsequence of proof step substitutions
typedef std::vector<Subprfstep> Subprfsteps;
// Pointers to the proofs to be included
typedef std::vector<Proofsteps const *> pProofs;
// Write the proof from pointers to proof of hypotheses.
void writeproof(Proofsteps & dest, Assptr pass, pProofs const & hyps);

// Map: equality syntax axioms -> its 3 justifications
#if __cplusplus >= 201103L
typedef std::map<const char *, const char *[3]> Equalities;
#else
typedef std::map<const char *, std::vector<const char *> > Equalities;
#endif // __cplusplus >= 201103L

#endif // STEP_H_INCLUDED
