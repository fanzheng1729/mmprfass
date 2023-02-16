#ifndef STMT_H_INCLUDED
#define STMT_H_INCLUDED

#include "strview.h"
#include "token.h"

const char stmttypes[] = "{}cvdfeap";

// Statement ($4.2)
struct Statement
{
    strview label;
    // type, -1: unfinished, -2: bad token
    int type;
    Tokens::size_type begin, end;
    operator bool() { return end != 0; }
    Statement(Tokens & tokens) : label(""), type(-1), begin(0), end(0)
    {
        if (tokens.empty())
            return;
        strview token(tokens.front());
        tokens.pop();
        // Check if it is a label.
        if (islabeltoken(token))
        {
            label = token;
            // Get statement type.
            if (tokens.empty())
                return;
            token = tokens.front();
            tokens.pop();
        }
        // Find statement type.
        const char * s(token.c_str);
        if (std::strlen(s) != 2 || s[0] != '$' || s[1] == '\0' ||
            !(s = std::strchr(stmttypes, s[1])))
        {
            type = -2;
            label = token;
            return;
        }
        type = s - stmttypes;
        // Find the span of the statement.
        begin = tokens.position;
        if (type <= 1) // Scoping statements.
        {
            end = begin + 1;
            return;
        }
        // Non-scoping statements. Find its end.
        for ( ; !tokens.empty(); tokens.pop())
        {
            token = tokens.front();
            if (token == "$.") break;
        }
        if (tokens.empty())
            return;
        end = tokens.position;
        tokens.pop();
    }
};

#endif // STMT_H_INCLUDED
