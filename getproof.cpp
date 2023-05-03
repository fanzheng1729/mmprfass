#include <algorithm>
#include <cctype>
#include "util/for.h"
#include "io.h"
#include "proof.h"
#include "token.h"

// Determine if there is no more token before finishing a statement.
bool unfinishedstat(Tokens const & tokens, strview stattype, strview label)
{
    if (tokens.empty())
        std::cerr << "Unfinished " << stattype << " statement " << label
                  << std::endl;
    return tokens.empty();
}

// Print error message indicating a proof is incomplete (-1) or bad (0).
int printbadprooferr(strview label, int err)
{
    static const char * const prefix[] = {"Warning: incomplete", "Error: no"};
    if (err != 1)
        std::cerr << prefix[err + 1] << " proof for theorem " << label << '\n';
    return err;
}

// Determine if a char is invalid in a compressed proof (Appendix B).
static bool badproofletter(unsigned char c)
{
    return !std::isupper(c) && c != '?';
}

// If a token is valid in a compressed proof (Appendix B), return 0.
// Otherwise return the bad letter.
static char badproofletters(strview token)
{
    const char * begin(token.c_str), * end(begin + std::strlen(begin));
    const char * s(std::find_if(begin, end, badproofletter));
    return s == end ? 0 : *s;
}

// Get sequence of letters in a compressed proof (Appendix B).
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
int getproofletters(strview label, Tokens & tokens, std::string & proof)
{
    strview token;

    while (!tokens.empty() && (token = tokens.front()) != "$.")
    {
        char c(badproofletters(token));
        if (c != 0)
        {
            std::cerr << "Bogus character " << c
                      << " in compressed proof of " << label << std::endl;
            return 0;
        }

        proof += token.c_str;
        tokens.pop();
    }

    if (unfinishedstat(tokens, "$p", label))
        return 0;

    tokens.pop(); // Discard $. token

    return printbadprooferr(label, proof.empty() ? 0 :
                            proof.find('?') != Token::npos ? -1 : 1);
}

// Subroutine for calculating proof number. Returns true iff okay.
template<unsigned MUL>
static bool addproofnumber(Proofnumber & num, int add, strview label)
{
    static const Proofnumber size_max(-1);

    if (num > size_max / MUL || MUL * num > size_max - add)
    {
        std::cerr << "Overflow computing numbers in compressed proof of "
                  << label << std::endl;
        return false;
    }

    num = MUL * num + add;
    return true;
}

// Get the raw numbers from compressed proof format.
// The letter Z is translated as 0 (Appendix B).
Proofnumbers getproofnumbers(strview label, std::string const & proof)
{
    Proofnumbers result;
    result.reserve(proof.size()); // Preallocate for efficiency

    std::size_t num(0u);
    bool justgotnum(false);
    FOR (char c, proof)
    {
        if (c <= 'T')
        {
            if (!addproofnumber<20>(num, c - ('A' - 1), label))
                return Proofnumbers();

            result.push_back(num);
            num = 0u;
            justgotnum = true;
        }
        else if (c <= 'Y')
        {
            if (!addproofnumber<5>(num, c - 'T', label))
                return Proofnumbers();

            justgotnum = false;
        }
        else // It must be Z
        {
            if (!justgotnum)
            {
                std::cerr << "Stray Z found in compressed proof of "
                          << label << std::endl;
                return Proofnumbers();
            }

            result.push_back(0u);
            justgotnum = false;
        }
    }

    if (num != 0u)
    {
        std::cerr << "Compressed proof of theorem " << label
                  << " ends in unfinished number" << std::endl;
        return Proofnumbers();
    }

    return result;
}
