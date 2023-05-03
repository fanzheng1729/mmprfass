#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

// Return the hypotheses of a goal to trim.
Bvector Prop::hypstotrim(Goalptr goalptr) const
{
    Assertion const & ass(m_assiter->second);
    Bvector result(ass.hypcount(), false);
    Hypsize ntotrim(0); // # essential hypothesis to trim
    for (Hypsize i(ass.hypcount() - 1); i != Hypsize(-1); --i)
    {
        if (ass.hypiters[i]->second.second)
            continue; // Skip floating hypotheses.
        // Try to trim the i-th hypothesis.
        result[i] = true;
        // Check if it can be trimmed.
        CNFClauses cnf2;
        std::vector<CNFClauses::size_type> const & ends(hypscnf.second);
        // Add hypotheses.
        for (Hypsize j(0); j < ass.hypcount(); ++j)
        {
            if (ass.hypiters[j]->second.second || result[j])
                continue; // Skip floating or unnecessary hypotheses.
            CNFClauses::size_type begin(j > 0 ? ends[j - 1] : 0), end(ends[j]);
            cnf2.insert(cnf2.end(), &hypscnf.first[begin], &hypscnf.first[end]);
        }
//        std::cout << "hypcnf\n" << hypscnf.first << "cnf\n" << cnf2;
        Atom natom(cnf2.empty() ? ass.hypcount() : cnf2.atomcount());
        // Add conclusion.
        m_database.propctors().addclause(goalptr->first, ass.hypiters, cnf2, natom);
        // Negate conclusion.
        cnf2.closeoff((natom - 1) * 2 + 1);
        ntotrim += result[i] = !cnf2.sat();
    }
    return ntotrim ? ass.trimvars(result, goalptr->first) : Bvector();
}

// Adds substitutions to a move.
struct Substadder : Adder
{
    Expression const & freevars;
    Moves & moves;
    Move & move;
    Environ const & env;
    Substadder(Expression const & freevars, Moves & moves, Move & move,
               Environ const & env) :
                   freevars(freevars), moves(moves), move(move), env(env) {}
    void operator()(Argtypes const & types, Genresult const & result,
                    Genstack const & stack)
    {
//std::cout << freevars << types << stack << std::endl;
        for (Proofsize i(0); i < types.size(); ++i)
            move.substitutions[freevars[i]] =
            result.at(freevars[i].typecode())[stack[i]];
        // Filter move by SAT.
        if (env.valid(move))
            moves.push_back(move);
    }
};

// Add a move with free variables.
void Prop::addhardmove(Assiter iter, Proofsize size, Move & move,
                       Moves & moves) const
{
    Assertion const & ass(iter->second);
    if (ass.nfreevar == 0 || size < ass.nfreevar)
        return;
    // Free variables in the theorem to be used
    Expression freevars;
    // Type codes of free variables
    Argtypes types;
    // Preallocate for efficiency.
    freevars.reserve(ass.nfreevar), types.reserve(ass.nfreevar);
    FOR (Varsused::const_reference var, ass.varsused)
        if (!var.second.back())
            freevars.push_back(var.first), types.push_back(var.first.typecode());
    // Generate substitution terms.
    Assertion const & thisass(m_assiter->second);
    FOR (Symbol3 var, freevars)
        generateupto(thisass.varsused, syntaxioms, var.typecode(), size,
                     genresult, termcounts);
    // Generate substitutions.
    Substadder adder(freevars, moves, move, *this);
    dogenerate(thisass.varsused, syntaxioms, ass.nfreevar, types, size+1,
               genresult, termcounts, adder);
//std::cout << moves;
}

// Test proof search for propositional theorems.
// Return the size of tree if okay. Otherwise return 0.
Prop::size_type testpropsearch
    (Assiter iter, Database const & database, Prop::size_type sizelimit,
     double const parameters[3])
{
    printass(*iter);
    Prop tree(iter, database, parameters);
    tree.play(sizelimit);
    // Check answer
    tree.printstats();
if (iter->first == "test181") tree.navigate();
    if (tree.size() > sizelimit)
    {
        std::cout << "Tree size limit exceeded. Main line:\n";
//        tree.printmainline();
//        tree.navigate();
    }
    else if (tree.value() != 1)
    {
        std::cerr << "Prop search test failed\n";
        tree.printmainline();
        return 0;
    }
    else if (!provesrightthing(iter->first,
                               verifyproofsteps(tree.proof(), &*iter),
                               iter->second.expression))
    {
        std::cerr << "Prop search test failed\n";
        std::cerr << tree.proof();
        tree.navigate();
    }
    else if (isonestep(tree.proof()))
    {
        const_cast<Assertion &>(iter->second).type |= Assertion::DUPLICATE;
        std::cout << "Duplicate of " << tree.proof().back() << std::endl;
    }
    else if (iter->first == "exp")
    {
        Printer printer(&database.typecodes());
        verifyproofsteps(tree.proof(), printer, &*iter);
        std::cout << printer.str(indentation(prooftree(tree.proof())));
        tree.navigate();
    }

    return tree.size();
}

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, Prop::size_type const sizelimit,
     double const parameters[3])
{
    std::cout << "Testing propositional proof search";
    Progress progress(std::cerr);
    Timer timer;
    bool okay(true);
    Prop::size_type nodecount(0);
    Assiters::size_type all(0), solved(0);
    // Test assertions.
    Assiters const & assiters(database.assvec());
    for (Assiters::size_type i(2700); i < assiters.size(); ++i)
    {
        Assiter iter(assiters[i]);
//        Assiter iter1 = database.assertions().find("biass");
//        iter = database.assertions().find("biluk");
//        const_cast<Assertion &>(iter->second).number = iter1->second.number+1;
        if (iter->second.type & Assertion::AXIOM)
            continue; // Skip axioms.
        if (!((Prop *)0)->Prop::ontopic(iter->second))
            continue; // Skip non propositional theorems.
        Prop::size_type const n(testpropsearch(iter, database, sizelimit,
                                               parameters));
        ++all;
        if (n == 0)
        {
            okay = false;
            break;
        }
        nodecount += n;
        solved += n <= sizelimit;
        progress << i/static_cast<double>(assiters.size() - 1);
    }
    // Collect statistics.
    double const t(timer);
    std::cout << '\n';
    std::cout << nodecount << " nodes / " << t << "s = ";
    std::cout << nodecount/t << " nps\n";
    std::cout << solved << '/' << all << " = ";
    std::cout << 100.0*solved/all << "% solved" << std::endl;
    return okay;
}
