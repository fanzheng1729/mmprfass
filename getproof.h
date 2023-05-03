#ifndef GETPROOF_H_INCLUDED
#define GETPROOF_H_INCLUDED

#include "strview.h"
class Tokens;

// Determine if there is no more token before finishing a statement.
bool unfinishedstat(Tokens const & tokens, strview stattype, strview label);

// Print error message indicating a proof is incomplete (-1) or bad (0).
int printbadprooferr(strview label, int err);

// Get sequence of letters in a compressed proof (Appendix B).
// Discard tokens up to and including $.
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
int getproofletters(strview label, Tokens & tokens, std::string & proof);

// Get the raw numbers from compressed proof format.
// The letter Z is translated as 0 (Appendix B).
Proofnumbers getproofnumbers(strview label, std::string const & proof);

#endif // GETPROOF_H_INCLUDED
