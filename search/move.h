#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include <iostream>
#include "../ass.h"
#include "../proof/verify.h"

// Move in proof search tree
struct Move;
// List of moves
typedef std::vector<Move> Moves;

// Move in proof search tree
struct Move
{
    enum Type { NONE, ASS, DEFER };
    typedef std::vector<Proofsteps> Substitutions;
    union
    {
        // Type of the attempt, if our turn
        Type type;
        // Index of the hypothesis, if not our turn
        Hypsize index;
    };
    // Pointer to the assertion to be used
    Assptr pass;
    // Substitutions to be used, if our turn
    Substitutions substitutions;
    Move(Type t = NONE) : type(t), pass(NULL) {}
    // A move applying an assertion, on our turn
    Move(Assptr ptr, Substitutions const & subst) :
        type(ASS), pass(ptr), substitutions(subst) {}
    // A move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pass(NULL) {}
    // Expression the attempt of using an assertion proves
    Proofsteps exprPolish() const
    {
        Proofsteps result;
        makesubstitution(pass->second.exprPolish, result, substitutions,
                         Proofstep::id);
        return result;
    }
    strview exptypecode() const { return pass->second.expression[0]; }
    // Hypothesis the attempt (must be of type ASS) needs
    Proofsteps hyprPolish(Hypsize index) const
    {
        Proofsteps result;
        makesubstitution(pass->second.hypsrPolish[index],
                         result, substitutions, Proofstep::id);
        return result;
    }
    strview hyptypecode(Hypsize index) const
    {
        return pass->second.hypotheses[index]->second.first[0];
    }
    // Find the index of hypothesis by expression.
    Hypsize matchhyp(Proofsteps const & proofsteps, strview typecode) const
    {
        for (Hypsize i(0); i < hypcount(); ++i)
            if (hyprPolish(i) == proofsteps && hyptypecode(i) == typecode)
                return i;

        return hypcount();
    }
    // # of hypotheses the attempt (must be of type ASS) needs
    Hypsize hypcount() const { return pass->second.hypcount(); }
    // # of variables the attempt (must be of type ASS) needs
    Symbol3s::size_type varcount() const { return pass->second.varcount(); }
    // Check if a hypothesis is floating
    bool isfloating(Hypsize index) const
    { return pass->second.hypotheses[index]->second.second; }
    // Return true if the assertion applied has no essential hypothesis.
    bool closes() const { return type == ASS && hypcount() == varcount(); }
    // Output the move (must be our move).
    friend std::ostream & operator<<(std::ostream & out, Move const & move)
    {
        static const char * const msg[] = {"NONE", "", "DEFER"};
        out << msg[move.type];
        if (move.type == ASS)
            out << move.pass->first.c_str;
        return out;
    }
};

#endif // MOVE_H_INCLUDED
