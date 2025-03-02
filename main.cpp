// Metamath database verifier
// Eric Schmidt (eric41293@comcast.net)
//
// I release this code to the public domain under the
// Creative Commons "CC0 1.0 Universal" Public Domain Dedication:
//
// http://creativecommons.org/publicdomain/zero/1.0/
//
// This is a standalone verifier for Metamath database files,
// written in portable C++. Run it with a single file name as the
// parameter.
//
// Some notes:
//
// The code assumes that the character set is compatible with ASCII.
//
// According to the spec, file inclusion commands should not include a file
// that has already been included. Unfortunately, determing whether two
// different strings refer to the same file is not easy, and, worse, is
// system-dependent. This program ignores the issue entirely and assumes
// that distinct strings name different files. This should be adequate for
// the present, at least.
//
// If the verifier finds an error, it will report it and quit. It will not
// attempt to recover and find more errors. The only condition that generates
// a diagnostic message but doesn't halt the program is an incomplete proof,
// specified by a question mark. In that case, as per the spec, a warning is
// issued and checking continues.
//
// Please let me know of any bugs.

//#include <fstream>
#include "database.h"
#include "io.h"
#include "search/prop.h"
#include "sect.h"
#include "test.h"
#include "util/timer.h"

    Database database;

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        std::cerr << "Syntax: mmprfass <filename> [<section title>]\n";
        return EXIT_FAILURE;
    }

    if (!pretest())
        return EXIT_FAILURE;

    Tokens tokens;
    Comments comments;
    std::cout << "Reading file ... ";
    Timer timer;
    // Read tokens. Returns true iff okay.
    bool read(const char * const filename, Tokens & tokens, Comments & comments);
    if (!read(argv[1], tokens, comments))
        return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;

    Sections sections(comments);
    if (!sections.empty())
        std::cout << "Last section: " << sections.rbegin()->first
                  << sections.rbegin()->second;

    tokens.position = 0;
    // Iterator to the end section
    Sections::const_iterator const end
        (argv[2] ? sections.find(argv[2]) : sections.end());
    // # tokens to read
    Tokens::size_type const size(end == sections.end() ? tokens.size() :
                                 end->second.tokenpos());
    std::cout << "Reading and verifying data";
    timer.reset();
    if (!database.read(tokens, comments, size))
        return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;

    if (!sections.empty())
        std::cout << "Last section: " << sections.rbegin()->first << std::endl;
    std::cout << "Variables: " << database.varvec();

    std::cout << "Checking iterators" << std::endl;
    if (!checkassiters(database.assertions(), database.assvec()))
        return EXIT_FAILURE;

    std::cout << "Parsing syntax trees";
    timer.reset();
    if (!database.rPolish())
        return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;
    std::cout << "Equality constructors: " << database.equalities();

    std::cout << "Checking syntax trees";
    timer.reset();
    if (!database.checkrPolish())
        return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;

    database.loaddefinitions();
    std::cout << "Defined syntax axioms\n" << database.definitions();
    std::cout << "Primitive syntax axioms\n" << database.primitivesyntaxioms();
    if (!database.checkdefinitions())
        return EXIT_FAILURE;

    database.loadpropasinfos();
    std::cout << "Syntax axioms with truth tables\n" << database.propctors();
    if (!database.propctors().okay(database.definitions()))
        return EXIT_FAILURE;

    std::cout << database.markpropassertions() << '/';
    std::cout << database.assertions().size() << " propositional assertions ";
    timer.reset();
    if (!database.checkpropassertion())
        return EXIT_FAILURE;
    std::cout << "checked in " << timer << 's' << std::endl;

    double parameters[] = {0, 1e-3, 0};
//    parameters[2] = SearchBase::STAGED;
//Uncomment the next two lines if you want to output to a file.
//    std::ofstream out("result.txt");
//    std::basic_streambuf<char> * sb(std::cout.rdbuf(out.rdbuf()));
    if (!testpropsearch(database, 1 << 10, parameters))
        return EXIT_FAILURE;
//Also please uncomment this line, or you will get a segmentation fault.
//    std::cout.rdbuf(sb);

//    std::cout << "Hard theorems" << std::endl;
//    database.printhardassertions(sections);

    return EXIT_SUCCESS;
}
