#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>
#include "comment.h"
#include "io.h"
#include "token.h"
#include "util/for.h"

// Return the first token in a string, or "" if there is not any.
static Token firsttoken(std::string const & str)
{
    std::string::size_type const begin(str.find_first_not_of(mmws));
    return begin == std::string::npos ? "" :
        str.substr(begin, str.find_first_of(mmws, begin) - begin);
}

// Classify comments ($4.4.2 and 4.4.3).
std::vector<strview> Comments::operator[](strview type) const
{
    std::vector<strview> result;

    FOR (const_reference comment, *this)
        if (firsttoken(comment.content) == type)
            result.push_back(comment.content);

    return result;
}

static void commenterr(const char * badstring, std::string const & comment)
{
    std::cerr << "Characters " << badstring << " found in a comment:\n";
    std::cerr << comment << std::endl;
}

// Read and return a comment. Return "" on failure ($4.1.2).
std::string comment(std::ifstream & in)
{
    std::stringstream result;//("", std::ios_base::ate);

    while (in.good())
    {
        // Read up to '$'. Skip if the next char is '$'. Otherwise in fails.
        in.get(*result.rdbuf(), '$');

        // Eat '$' in the stream.
        if (in.eof() || in.bad() || (in.clear(), in.get(), !in.good()))
        {
            std::cerr << "Unclosed comment" << result.str() << std::endl;
            return "";
        }

        // Check if the token is legal.
        char c(in.get());
        if (c == '(')
        {
            commenterr("$(", result.str());
            return "";
        }

        if (c == ')')
        {
            // the last char read
            char last(0);
            result.seekg(-1, std::ios::end);
            result >> last;

            if (std::strchr(mmws, last) == NULL)
            {
                commenterr("...$)", result.str());
                return "";
            }

            // The token begins with "$)". Check if it ends here.
            if (in.eof())
                return result.str();
            if (!in.good())
                return "";

            // the next char to read
            if (std::strchr(mmws, in.peek()) == NULL)
            {
                commenterr("$)...", result.str());
                return "";
            }

            // "$)" is legal.
            return result.str();
        }

        // "$" followed by other chars
        result.seekg(0, std::ios::end);
        result << '$' << c;
    }

    return "";
}

// Read commands from comments ($4.4.2 and 4.4.3). C-comment unsupported.
void Commands::addcmd(std::string str)
{
    char * token(std::strtok(&str[0], mmws));
    if (!token || std::strlen(token) != 2 || token[0] != '$')
        return;
    // metamath comment types ($4.4.2 and $4.4.3)
    if (std::strchr("jt", token[1]) == NULL)
        return;
    // Move to next token.
    char * tokenend(token + std::strlen(token));
    if (tokenend < str.data() + str.size())
        ++tokenend;

    resize(size() + 1);
    while ((token = std::strtok(tokenend, mmws)))
    {
        // end of token.
        tokenend = token + std::strlen(token);
        // Break token at ';'.
        char * s(std::strtok(token, ";"));
        while (s)
        {
            // Add token.
            back().push_back(s);
            // Check if there is a semicolon.
            if (s + std::strlen(s) < tokenend)
                resize(size() + 1);
            // Find new token.
            s = std::strtok(NULL, ";");
        }
        // Move to next token.
        if (tokenend < str.data() + str.size())
            ++tokenend;
    }
    // Remove possible empty command.
    if (back().empty()) pop_back();
    return;
}

Commands::Commands(std::vector<strview> const & comments)
{
    FOR (strview comment, comments)
        addcmd(comment);
}

// Classify comments ($4.4.3).
Commands Commands::operator[](strview type) const
{
    Commands result;

    FOR (const_reference command, *this)
        if (!command.empty() && command.front() == type)
        {
            result.resize(result.size() + 1);
            result.back().assign(command.begin() + 1, command.end());
        }

    return result;
}

// Return the unquoted word. Return "" if it is not quoted.
static std::string unquote(std::string const & str)
{
    if (str.size() < 3 || str[0] != '\'' || str[str.size() - 1] != '\'')
        return "";

    return str.substr(1, str.size() - 2);
}

std::ostream & operator<<(std::ostream & out, Commands commands)
{
    FOR (Command const & command, commands)
        out << command;

    return out;
}

Typecodes::Typecodes(Commands const & syntax, Commands const & bound)
{
    FOR (Command const & command, syntax)
        addsyntax(command);
    FOR (Command const & command, bound)
        addbound(command);
}

// Check if a type code is primitive. Return -1 if not found.
int Typecodes::isprimitive(std::string const & typecode) const
{
    const_iterator const iter(find(typecode));
    return iter == end() ? -1 : iter->second.first.empty() ? 1 : 0;
}

// Return the normalized type code. Return "" if not found.
strview Typecodes::normalize(strview typecode) const
{
    const_iterator iter(find(typecode));
    if (iter == end()) return "";

    while (!iter->second.first.empty())
        iter = find(iter->second.first);

    return iter->first;
}

void Typecodes::addsyntax(Command const & command)
{
    if (unexpected(command.size() != 1 &&
                   !(command.size() == 3 && command[1] == "as"),
                   "syntax command", command))
        return;
    // Get the type.
    std::string const & type(unquote(command.front()));
    if (unexpected(type.empty(), "type code", command.front()))
        return;
    // Add the type.
    std::pair<iterator, bool> result(insert(value_type(type, mapped_type())));
    if (result.second == false)
    {
        std::cerr << "Type code " << type << " already exists\n";
        return;
    }
    // No as type.
    if (command.size() == 1)
        return;
    // Add the as type.
    std::string const & astype(unquote(command.back()));
    if (unexpected(astype.empty(), "type code", command.back()))
        return;
    if (count(astype) == 0)
    {
        std::cerr << "Type code " << astype << " does not exist\n";
        return;
    }
    result.first->second.first = astype;
}

void Typecodes::addbound(Command const & command)
{
    if (unexpected(command.size() != 1, "bound syntax command", command))
        return;

    iterator const iter(find(unquote(command[0])));
    if (unexpected(iter == end(), "type code", command[0]))
        return;

    iter->second.second = true;
}

Ctordefns::Ctordefns(Commands const & definitions, Commands const & primitives)
{
    FOR (Command const & command, definitions)
        adddefinition(command);
    FOR (Command const & command, primitives)
        addprimitives(command);
}

void Ctordefns::adddefinition(Command const & command)
{
    if (unexpected(command.size() != 3
                   || command[1] != "for", "command", command))
        return;
    // Get the constructor.
    std::string const & ctor(unquote(command.back()));
    if (unexpected(ctor.empty(), "constructor", command.back()))
        return;
    // Get its definition.
    std::string const & defn(unquote(command.front()));
    if (unexpected(defn.empty(), "definition", command.front()))
        return;
    // Add the constructor.
    if (insert(value_type(ctor, defn)).second == false)
        std::cerr << "Constructor " << ctor << " already exists\n";
}

void Ctordefns::addprimitives(Command const & command)
{
    FOR (std::string const & token, command)
    {
        // Get the constructor.
        std::string const & ctor(unquote(token));
        if (unexpected(ctor.empty(), "constructor", token))
            return;
        // Add the constructor.
        if (insert(value_type(ctor, "")).second == false)
            std::cerr << "Constructor " << ctor << " already exists\n";
    }
}

Commentinfo::Commentinfo(Comments const & comments)
{
    if (comments.empty())
        return;

//std::cout << "$j comments\n" << comments["$j"];
    Commands const commands(comments["$j"]);
//std::cout << "$j commands\n" << commands;
    typecodes = Typecodes(commands["syntax"], commands["bound"]);
//std::cout << "Syntax type codes: " << typecodes;
//std::cout << "Bound type codes: " << commands["bound"];
    ctordefns = Ctordefns(commands["definition"], commands["primitive"]);
//std::cout << "Constructor definitions: " << ctordefns;
}
