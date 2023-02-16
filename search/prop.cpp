#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

// Evaluate leaf nodes, and record the proof if proven.
Prop::Eval Prop::evaltheirleaf(Node const & node) const
{
    if (node.attempt.type != Move::ASS)
        return eval(hypslen + node.pgoal->first.size() + node.defercount());
//std::cout << "Evaluating " << node;
    double eval(1);
    for (Hypsize i(0); i < node.attempt.hypcount(); ++i)
    {
        if (node.attempt.isfloating(i))
            continue; // Skip floating hypothesis.
        Goals::pointer pgoal(node.hypvec[i]);
        if (done(pgoal, node.attempt.hyptypecode(i)))
            continue; // Skip proven goals.
        bool const needshyps(pgoal->second.hypsneeded);
        double const neweval(score(needshyps * hypslen + pgoal->first.size()));
        if (neweval < eval)
            eval = neweval;
    }
    if (eval == 1)
        writeproof(node);
    return Eval(eval, eval == 1);
}

// Check if all hypotheses of a move are valid.
bool Prop::valid(Move const & move) const
{
    for (Hypsize i(0); i < move.hypcount(); ++i)
    {
        if (move.isfloating(i))
            continue; // Skip floating hypothesis.

        Proofsteps const & goal(move.hyprPolish(i));
        Goals::pointer pgoal(((Environ &)(*this)).addgoal(goal, NEW));
        Value const value(pgoal->second.value);
        if (value == PROVEN || value == PENDING)
            continue; // goal checked to be true
        if (value == FALSE)
            return false; // goal checked to be false
        // New goal
        const bool okay(valid(goal));
        pgoal->second.value = okay ? PENDING : FALSE;
        if (!okay) // Invalid goal found
            return false;
        pgoal->second.hypsneeded = hypsneeded(goal);
//std::cout << goal << (!pgoal->second.hypsneeded ? "un" : "") << "used for " << goal;
    }

    return true;
}

// Add Hypothesis-oriented moves.
void Prop::addhypmoves(Move const & move, Moves & moves,
                       Subprfsteps const & subprfsteps) const
{
    Assertion const & thm(move.pass->second);
    Assertion const & ass(m_assiter->second);
    // Iterate through key hypotheses i of the theorem.
    FOR (Hypsize i, thm.keyhyps)
    {
        Hypiter const hypi(thm.hypotheses[i]);
//std::cout << move.pass->first << ' ' << hypi->first << ' ';
//std::cout << thm.hypsrPolish[i] << move.hyprPolish(i);
//std::cin.get();
        for (Hypsize j(0); j < ass.hypcount(); ++j)
        {
            Hypiter const hypj(ass.hypotheses[j]);
            if (hypj->second.second)
                continue; // Skip floating hypotheses.
            // Match hypothesis j against key hypothesis i of the theorem.
            Subprfsteps newsub(subprfsteps);
            if (findsubstitutions(ass.hypsrPolish[j], ass.hypstree[j],
                                  thm.hypsrPolish[i], thm.hypstree[i],
                                  newsub))
            {
//std::cout << hypj->first << ' ' << ass.hypsrPolish[j];
                // Free substitutions from the key hypothesis
                Move::Substitutions substitutions(subprfsteps.size());
                for (Subprfsteps::size_type i(1); i < newsub.size(); ++i)
                    substitutions[i] = Proofsteps(newsub[i].first,
                                                  newsub[i].second);
                Move newmove(move.pass, substitutions);
                if (valid(newmove))
                    moves.push_back(newmove);
//std::cin.get();
            }
        }
    }
}

// Adds substitutions to a move.
struct Substadder
{
    Expression const & freevars;
    Moves & moves;
    Move & move;
    Prop const & prop;
    void operator()(Argtypes const & types, Genresult const & result,
                    Genstack const & stack)
    {
//std::cout << freevars << types << stack << std::endl;
        for (Proofsize i(0); i < types.size(); ++i)
            move.substitutions[freevars[i]] =
            result.at(freevars[i].typecode())[stack[i]];
        // Filter move by SAT.
        if (prop.valid(move))
            moves.push_back(move);
    }
};

// Add a move with free variables.
void Prop::addhardmove(Assiter iter, Proofsize size, Move & move,
                       Moves & moves) const
{
    if (size == 0) return;
    Assertion const & ass(iter->second);
    Proofsize const nfreevars(ass.varcount() - ass.expvarcount());
    if (nfreevars == 0 || size < nfreevars)
        return;

    // Free variables in the theorem to be used
    Expression freevars;
    // Type codes of free variables
    Argtypes types;
    // Preallocate for efficiency.
    freevars.reserve(nfreevars), types.reserve(nfreevars);
    FOR (Varsused::const_reference var, iter->second.varsused)
        if (!var.second.back())
            freevars.push_back(var.first), types.push_back(var.first.typecode());

    // Generate substitution terms.
    FOR (Symbol3 var, freevars)
        generateupto(varsused, syntaxioms, var.typecode(), size,
                     genresult, termcounts);
    // Generate substitutions.
    Substadder adder = {freevars, moves, move, *this};
    dogenerate(varsused, syntaxioms, nfreevars, types, size+1,
               genresult, termcounts, adder);
//std::cout << moves;
}

// Try applying the assertion, and add moves if successful.
// Return true iff a move closes the goal.
bool Prop::tryassertion
    (Goal goal, Prooftree const & tree, Assiter iter, Proofsize size,
     Moves & moves) const
{
    Assertion const & ass(iter->second);
    if (ass.expression.empty() || ass.expression[0] != goal.typecode)
        return false; // Type code mismatch
//std::cout << "Trying " << iter->first << " with " << goal.expression();
    Subprfsteps subprfsteps;
    prealloc(subprfsteps, ass.varsused);
    if (!findsubstitutions(*goal.prPolish, tree, ass.exprPolish, ass.exptree,
                           subprfsteps))
        return false; // Conclusion mismatch
    // Bound substitutions
    Move::Substitutions substitutions(subprfsteps.size());
    for (Subprfsteps::size_type i(1); i < subprfsteps.size(); ++i)
        substitutions[i] = Proofsteps(subprfsteps[i].first,
                                      subprfsteps[i].second);
    // Move with all bound substitutions
    Move move(&*iter, substitutions);
    if (size == 0)
    {
        if (ass.varcount() == ass.expvarcount())
            // No free variable
            return addeasymove(move, moves);
        // Hypothesis-oriented moves
        addhypmoves(move, moves, subprfsteps);
        return false;
    }
    // size > 0
    addhardmove(iter, size, move, moves);
    return false;
}

// Moves with a given size
Moves Prop::ourmovesbysize(Node const & node, Proofsize size) const
{
    Assiters const & assvec(m_database.assvec());
    Prooftree tree(prooftree(node.pgoal->first));
    if (unexpected(tree.empty(), "corrupt proof tree when proving goal",
                   node.pgoal->first))
        return Moves();
    Moves moves;
//std::cout << "Finding moves for " << node;
    for (Assiters::size_type i
         (1); i < m_assiter->second.number && i < assvec.size(); ++i)
    {
        Assiter iter(assvec[i]);
        Assertion const & ass(iter->second);
        if (!(ass.type & Assertion::TRIVIAL) &&
            ((ass.varcount() > ass.expvarcount()) == (size > 0) ||
             (!ass.keyhyps.empty() && size == 0)))
            tryassertion(node.goal(), tree, iter, size, moves);
    }
//std::cout << moves;
    return moves;
}

// Test proof search for propositional assertions.
// Return the size of tree if okay. Otherwise return 0.
Prop::size_type testpropsearch
    (Assiter iter, Database const & database, Prop::size_type sizelimit,
     double const parameters[3])
{
printass(*iter);
    Prop tree(iter, database, parameters);
    tree.play(sizelimit);
//if (iter->first == "imim2i") tree.navigate();
    // Check answer
tree.printstats();
    if (tree.size() > sizelimit)
    {
        std::cout << "Tree size limit exceeded. Main line:\n";
//        tree.printmainline(false);
//        tree.navigate();
    }
    else if (tree.value() != 1)
    {
        std::cerr << "Prop search test failed\n";
        tree.printfulltree();
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
    for (Assiters::size_type i(1); i < assiters.size(); ++i)
    {
        Assiter iter(assiters[i]);
        if (iter->second.type & Assertion::AXIOM)
            continue; // Skip axioms.
        if (!(iter->second.type & Assertion::PROPOSITIONAL))
            continue; // Skip non propositional theorems.
        Prop::size_type const n(testpropsearch(iter, database, sizelimit,
                                               parameters));
        ++all;
        if (n == 0)
        {
            okay = false;
            nodecount += sizelimit + 1;
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
    std::cout << 100.*solved/all << "% solved" << std::endl;
    return okay;
}
