#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

// Return the hypotheses of a goal to trim.
Bvector Prop::hypstotrim(Goalptr goalptr) const
{
    Bvector result(m_ass.hypcount(), false);

    Hypsize ntotrim(0); // # essential hypothesis to trim
    for (Hypsize i(m_ass.hypcount() - 1); i != Hypsize(-1); --i)
    {
        if (m_ass.hypiters[i]->second.second)
            continue; // Skip floating hypotheses.
        // Try to trim the i-th hypothesis.
        result[i] = true;
        // Check if it can be trimmed.
        CNFClauses cnf2;
        std::vector<CNFClauses::size_type> const & ends(hypscnf.second);
        // Add hypotheses.
        for (Hypsize j(0); j < m_ass.hypcount(); ++j)
        {
            if (m_ass.hypiters[j]->second.second || result[j])
                continue; // Skip floating or unnecessary hypotheses.
            CNFClauses::size_type begin(j > 0 ? ends[j - 1] : 0), end(ends[j]);
            cnf2.insert(cnf2.end(), &hypscnf.first[begin], &hypscnf.first[end]);
        }
//        std::cout << "hypcnf\n" << hypscnf.first << "cnf\n" << cnf2;
        Atom natom(cnf2.empty() ? m_ass.hypcount() : cnf2.atomcount());
        // Add conclusion.
        m_database.propctors().addclause(goalptr->first, m_ass.hypiters, cnf2, natom);
        // Negate conclusion.
        cnf2.closeoff((natom - 1) * 2 + 1);
        ntotrim += result[i] = !cnf2.sat();
    }

    return ntotrim ? m_ass.trimvars(result, goalptr->first) : Bvector();
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

// Add a move with free variables. Return false.
bool Prop::addhardmoves(Assiter iter, Proofsize size, Move & move,
                       Moves & moves) const
{
    Assertion const & thm(iter->second);
    // Free variables in the theorem to be used
    Expression freevars;
    // Type codes of free variables
    Argtypes types;
    // Preallocate for efficiency.
    freevars.reserve(thm.nfreevar), types.reserve(thm.nfreevar);
    FOR (Varsused::const_reference var, thm.varsused)
        if (!var.second.back())
            freevars.push_back(var.first), types.push_back(var.first.typecode());
    // Generate substitution terms.
    FOR (Symbol3 var, freevars)
        generateupto(m_ass.varsused, syntaxioms, var.typecode(), size,
                     genresult, termcounts);
    // Generate substitutions.
    Substadder adder(freevars, moves, move, *this);
    dogenerate(m_ass.varsused, syntaxioms, types, size + 1,
               genresult, termcounts, adder);
//std::cout << moves;
    return false;
}

// Test proof search for propositional theorems.
// Return the size of tree if okay. Otherwise return 0.
Prop::size_type testpropsearch
    (Assiter iter, Database const & database, Prop::size_type sizelimit,
     double const parameters[3])
{
//    printass(*iter);
    Prop tree(iter->second, database, parameters);
    tree.play(sizelimit);
    // Check answer
//    tree.printstats();
//if (iter->first == "test181") tree.navigate();
    if (tree.size() > sizelimit)
    {
//        std::cout << "Tree size limit exceeded. Main line:\n";
//        tree.printmainline();
//        tree.navigate();
    }
    else if (tree.value() != 1)
    {
        std::cerr << "Prop search returned " << tree.value() << "\n";
        tree.printmainline();
        return 0;
    }
    else if (!provesrightthing(iter->first,
                               verifyproofsteps(tree.proof(), &*iter),
                               iter->second.expression))
    {
        std::cerr << "Wrong proof: " << tree.proof();
        tree.navigate();
    }
    else if (iter->first == "test8")
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
    Assiters::size_type all(0), proven(0);
    // Test assertions
    Assiters const & assiters(database.assvec());
    for (Assiters::size_type i(1); i < assiters.size(); ++i)
    {
        Assiter iter(assiters[i]);

        // Skip axioms
        if (iter->second.type & Asstype::AXIOM)
            continue;

        // Skip non propositional theorems
        if (!(static_cast<Prop *>(0))->Prop::ontopic(iter->second))
            continue;

        // Skip trivial theorems.
        Prop tree(iter->second, database, parameters);
        if (tree.evalleaf(tree.root()) == Eval(WDL::WIN, true))
            continue;

        // Skip duplicate theorems
        tree.playonce();
        if (tree.value() == WDL::WIN)
        {
            const_cast<Assertion &>(iter->second).type |= Asstype::DUPLICATE;
            continue;
        }

        // Try search proof.
        Prop::size_type const n(testpropsearch(iter, database, sizelimit,
                                               parameters));
        ++all;
        if (n == 0)
        {
            okay = false;
            break;
        }
        nodecount += n;
        proven += n <= sizelimit;
        progress << i/static_cast<double>(assiters.size() - 1);
    }
    // Collect statistics.
    double const t(timer);
    std::cout << '\n';
    std::cout << nodecount << " nodes / " << t << "s = ";
    std::cout << nodecount/t << " nps\n";
    std::cout << proven << '/' << all << " = ";
    std::cout << static_cast<double>(100*proven)/all << "% proven" << std::endl;
    return okay;
}
