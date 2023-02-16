#ifndef COMMENT_H_INCLUDED
#define COMMENT_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <map>
#include <utility>
#include <vector>
#include "strview.h"

const char mmws[] = "\t\n\f\r "; // metamath whitespace chars ($4.1.1)

struct Comment
{
    std::string content;
    std::size_t position;
};

struct Comments : std::vector<Comment>
{
    using vector::operator[];
// Classify comments ($4.4.2 and 4.4.3).
    std::vector<strview> operator[](strview type) const;
};

// Read and return a comment. Return "" on failure ($4.1.2).
std::string comment(std::ifstream & in);

typedef std::vector<std::string> Command;

struct Commands : std::vector<Command>
{
// Read commands from comments ($4.4.2 and 4.4.3). C-comment unsupported.
    void addcmd(std::string str);
    Commands(std::vector<strview> const & comments = std::vector<strview>());
    using vector::operator[];
// Classify comments ($4.4.3).
    Commands operator[](strview type) const;
};

// Map: type code -> (as type code, or "" if none, is bound type code) ($4.4.3)
struct Typecodes : std::map<std::string, std::pair<std::string, bool> >
{
    Typecodes(Commands const & syntax = Commands(),
              Commands const & bound = Commands());
    // Check if a type code is primitive. Return -1 if not found.
    int isprimitive(std::string const & typecode) const;
    // Return the normalized type code. Return "" if not found.
    strview normalize(strview typecode) const;
private:
    void addsyntax(Command const & command);
    void addbound(Command const & command);
};

// Map: constructor -> definition, or "" if none ($4.4.3)
struct Ctordefns : std::map<std::string, std::string>
{
    Ctordefns(Commands const & definitions = Commands(),
              Commands const & primitives = Commands());
private:
    void adddefinition(Command const & command);
    void addprimitives(Command const & command);
};

struct Commentinfo
{
    Typecodes typecodes;
    Ctordefns ctordefns;
    Commentinfo(Comments const & comments = Comments());
};

#endif // COMMENT_H_INCLUDED
