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

#include <cstdlib>
#include <fstream>
#include "database.h"
#include "io.h"
#include "search/prop.h"
#include "stmt.h"
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

    std::cout << "Checking utilities" << std::endl;
    if (testlog2() || !testfilter(8))
        return EXIT_FAILURE;

    std::cout << "Checking SAT solvers: " << testsat1() << std::endl;
    if (testsat2(8) != 0)
        return EXIT_FAILURE;

//    std::cout << "Checking DAG" << std::endl;
//    if (!testDAG(8))
//        return EXIT_FAILURE;

//    std::cout << "Testing MTCS" << std::endl;
//    double const exploration[] = {1, 1};
//    if (!testMCTS(1 << 20, exploration))
//        return EXIT_FAILURE;

    Tokens tokens;
    Comments comments;
    std::cout << "Reading file" << std::endl;
    Timer timer;
    // Read tokens. Returns true iff okay.
    bool read(const char * const filename,
              Tokens & tokens, Comments & comments);
    if (!read(argv[1], tokens, comments))
        return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;
    std::cout << tokens.size() << " tokens, ";
    std::cout << comments.size() << " comments" << std::endl;

    Sections sections(comments);
    if (!sections.empty())
        std::cout << "Last section: " << sections.rbegin()->first
                  << sections.rbegin()->second;

    std::vector<Statement> statements;
    while (!tokens.empty()) statements.push_back(Statement(tokens));
    std::cout << statements.size() << " statements read, last one is ";
    std::cout << (statements.back() ? "good" : "bad") << std::endl;
    tokens.position = 0;

    if (!database.read(sections, tokens, comments, argv[2]))
        return EXIT_FAILURE;

    if (!sections.empty())
        std::cout << "Last section: " << sections.rbegin()->first << '\t'
                  << sections.rbegin()->second.assnumber << std::endl;
    std::cout << "Variables: " << database.varvec();

    std::cout << "Checking iterators" << std::endl;
    if (!checkassiters(database.assertions(), database.assvec()))
        return EXIT_FAILURE;

    if (!database.rPolish())
        return EXIT_FAILURE;
    std::cout << "Equality constructors: " << database.equalities();
    if (!database.checkrPolish())
        return EXIT_FAILURE;

    database.loaddefinitions();
    std::cout << "Defined syntax axioms\n" << database.definitions();
    std::cout << "Primitive syntax axioms\n" << database.primitivesyntaxioms();
    if (!database.checkdefinitions())
        return EXIT_FAILURE;

    database.loadpropasinfos();
    std::cout << "Syntax axioms with truth tables\n" << database.propctors();
    if (!database.propctors().okay(database.definitions()))
        return EXIT_FAILURE;

    std::cout << "Marking " << database.markpropassertions() << '/';
    std::cout << database.assertions().size() << " propositional assertions\n";
    if (!database.checkpropassertion())
        return EXIT_FAILURE;

    double parameters[] = {0, 1e-3, 0};
//    parameters[2] = SearchBase::STAGED;
//Uncomment the next two lines if you want to output to a file.
//    std::ofstream out("result.txt");
//    std::basic_streambuf<char> * sb(std::cout.rdbuf(out.rdbuf()));
    if (!testpropsearch(database, 8 << 10, parameters))
        return EXIT_FAILURE;
//Also please uncomment this line, or you will get a segmentation fault.
//    std::cout.rdbuf(sb);

//    std::cout << "Hard theorems" << std::endl;
//    database.printhardassertions(sections);

    return EXIT_SUCCESS;
}
