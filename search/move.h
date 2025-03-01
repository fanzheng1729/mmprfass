#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include <iostream>
#include "../ass.h"
#include "goal.h"

// Move in proof search tree
struct Move
{
    enum Type { NONE, ASS, DEFER };
    typedef std::vector<Proofsteps> Substitutions;
    union
    {
        // Type of the attempt, on our turn
        Type type;
        // Index of the hypothesis, on their turn
        Hypsize index;
    };
    // Pointer to the assertion to be used, on our turn
    Assptr pass;
    // Substitutions to be used, on our turn
    Substitutions substitutions;
    // Essential hypotheses needed, on our turn
    std::vector<Goalptr> mutable hypvec;
    Move(Type t = NONE) : type(t), pass(NULL) {}
    // A move applying an assertion, on our turn
    Move(Assptr ptr, Substitutions const & subst) :
        type(ASS), pass(ptr), substitutions(subst) {}
    // A move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pass(NULL) {}
    // Expression the attempt of using an assertion proves (must be of type ASS)
    Proofsteps exprPolish() const
    {
        Proofsteps result;
        makesubstitution(pass->second.exprPolish, result, substitutions,
                         Proofstep::id);
        return result;
    }
    strview exptypecode() const { return pass->second.expression[0]; }
    // Hypothesis (must be of type ASS)
    Hypiter hypiter(Hypsize index) const { return pass->second.hypiters[index]; }
    strview hyptypecode(Hypsize index) const
    { return hypiter(index)->second.first[0]; }
    bool isfloating(Hypsize index) const { return hypiter(index)->second.second; }
    // Hypothesis the attempt (must be of type ASS) needs
    Proofsteps hyprPolish(Hypsize index) const
    {
        Proofsteps result;
        makesubstitution(pass->second.hypsrPolish[index],
                         result, substitutions, Proofstep::id);
        return result;
    }
    // Find the index of hypothesis by expression.
    Hypsize matchhyp(Proofsteps const & proofsteps, strview typecode) const
    {
        Hypsize i(0);
        for ( ; i < hypcount(); ++i)
            if (hyprPolish(i) == proofsteps && hyptypecode(i) == typecode)
                return i;
        return i;
    }
    // # of hypotheses the attempt (must be of type ASS) needs
    Hypsize hypcount() const { return pass->second.hypcount(); }
    // # of variables the attempt (must be of type ASS) needs
    Symbol3s::size_type varcount() const { return pass->second.varcount(); }
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

// List of moves
typedef std::vector<Move> Moves;

#endif // MOVE_H_INCLUDED
