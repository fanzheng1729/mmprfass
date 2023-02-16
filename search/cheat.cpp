#include "../ass.h"
#include "cheat.h"
#include "../comment.h"
#include "../io.h"
#include "../msg.h"
#include "../proof/verify.h"

// Return the cheat sheet for a proof.
Cheatsheet::Cheatsheet(Assiter assiter, Typecodes const & typecodes)
{
    std::vector<Expression> stack, savedsteps;

    FOR (Proofstep step, assiter->second.proofsteps)
    {
        switch (step.type)
        {
        case Proofstep::HYP:
//std::cout << "Pushing hypothesis: " << step.phyp->first << '\n';
            stack.push_back(step.phyp->second.first);
            break;
        case Proofstep::ASS:
        {
            strview label(assiter->first), reflabel(step.pass->first);
//std::cout << "Applying assertion: " << reflabel << std::endl;
            Assertion const & ass(step.pass->second);
            // Find the necessary substitutions.
            Move::Substitutions substitutions;
            prealloc(substitutions, ass.varsused);
            std::vector<Expression>::size_type const base = findsubstitutions
                (label, reflabel, ass.hypotheses, stack, substitutions);
            if (base == stack.size())
            {
                clear();
                return;
            }
            // Insert new statement onto stack.
            makesubstitution(ass.expression, stack.back(), substitutions);

            if (!typecodes.isprimitive(ass.expression[0]))
            {
                // Add move to the cheat sheet.
                Move & move((*this)[stack.back()]);
                if (move.type != Move::NONE && !ass.hypotheses.empty())
                    std::cout << "Proved twice: " << stack.back();
                else
                    move = Move(step.pass, substitutions);
            }
            // Remove hypotheses from stack.
            stack.erase(stack.begin() + base, stack.end() - 1);
        }
            break;
        case Proofstep::LOAD:
            if (!enoughsavedsteps(step.index, savedsteps.size(), assiter->first))
            {
                clear();
                return;
            }
            stack.push_back(savedsteps[step.index]);
//std::cout << "Loaded saved step " << step.index << std::endl;
            break;
        case Proofstep::SAVE:
//std::cout << "Saving step " << savedsteps.size() << std::endl;
            if (stack.empty())
            {
                std::cerr << "No step to save in compressed proof\n";
                clear();
                return;
            }
            savedsteps.push_back(stack.back());
            break;
        default:
            std::cerr << "Invalid step in compressed proof\n";
            clear();
            return;
        }
    }

    if (stack.size() != 1)
    {
        std::cerr << "Proof of theorem" << stackszerr << std::endl;
        clear();
        return;
    }
}

void Cheatsheet::print(bool onlyfreesubstitution) const
{
    FOR (const_reference r, *this)
    {
        if (Assptr pass = r.second.pass)
        {
            Assertion const & thm(pass->second);
            if (thm.expvarcount() == thm.varcount() && onlyfreesubstitution)
                continue;
            std::cout << pass->first << ' ' << thm.expvarcount() << ',' << thm.varcount();
        }
        std::cout << '\t' << r.first;
    }
}

// Print the cheat sheet of all assertions.
void printcheatsheets(Assiters const & assiters, Typecodes const & typecodes)
{
    for (Assiters::size_type i(1); i < assiters.size(); ++i)
    {
        Assiter iter(assiters[i]);
        if (iter->second.proofsteps.empty())
            continue; // Skip assertions without proof.

        Cheatsheet cheatsheet(iter, typecodes);
        if (!cheatsheet.hashardsubstitution(ismovehard))
            continue;

        printass(*iter);
        std::cout << std::endl;
        cheatsheet.print(true);
        std::cin.get();
    }
}
/*
// Return the cheat sheet for a proof.
Cheatsheet2::Cheatsheet2(Assiter assiter, Typecodes const & typecodes)
{
    std::vector<Proofsteps> stack, savedsteps;

    FOR (Proofstep step, assiter->second.proofsteps)
    {
        switch (step.type)
        {
        case Proofstep::HYP:
//std::cout << "Pushing hypothesis: " << step.phyp->first << '\n';
            if (step.phyp->second.second)
            {
                // Floating hypothesis
                stack.push_back(Proofsteps(1, step));
                break;
            }
            // Essential hypothesis
            for (Hypsize i(0); i < assiter->second.hypcount(); ++i)
                if (&*assiter->second.hypotheses[i] == step.phyp)
                {
                    stack.push_back(assiter->second.hypsrPolish[i]);
                    break;
                }
            unexpected(true, "hypothesis", step.phyp->first);
            clear();
            return;
        case Proofstep::ASS:
        {
            strview label(assiter->first), reflabel(step.pass->first);
//std::cout << "Applying assertion: " << reflabel << std::endl;
            Assertion const & ass(step.pass->second);
            // Find the necessary substitutions.
            Move::Substitusteps substitusteps;
            prealloc(substitusteps, ass.varsused);
            std::vector<Proofsteps>::size_type const base = findsubstitutions
                (label, reflabel, ass.hypotheses, stack, substitusteps);
            if (base == stack.size())
            {
                clear();
                return;
            }
            // Insert new statement onto stack.
            makesubstitution(ass.exprPolish, stack.back(), substitusteps, &Proofstep::id);

            if (!typecodes.isprimitive(ass.expression[0]))
            {
                // Add move to the cheat sheet.
                Move & move((*this)[stack.back()]);
                if (move.type != Move::NONE && !ass.hypotheses.empty())
                    std::cout << "Proved twice: " << stack.back();
                else
                    move = Move(step.pass, substitusteps);
            }
            // Remove hypotheses from stack.
            stack.erase(stack.begin() + base, stack.end() - 1);
        }
            break;
        case Proofstep::LOAD:
            if (!enoughsavedsteps(step.index, savedsteps.size(), assiter->first))
            {
                clear();
                return;
            }
            stack.push_back(savedsteps[step.index]);
//std::cout << "Loaded saved step " << step.index << std::endl;
            break;
        case Proofstep::SAVE:
//std::cout << "Saving step " << savedsteps.size() << std::endl;
            if (stack.empty())
            {
                std::cerr << "No step to save in compressed proof\n";
                clear();
                return;
            }
            savedsteps.push_back(stack.back());
            break;
        default:
            std::cerr << "Invalid step in compressed proof\n";
            clear();
            return;
        }
    }

    if (stack.size() != 1)
    {
        std::cerr << "Proof of theorem" << stackszerr << std::endl;
        clear();
        return;
    }
}
*/
// Test proof search for the assertion.
// Return the size of tree if okay. Otherwise return 0.
static Cheater::size_type testproofsearch
    (Assiter iter, Typecodes const & typecodes,
     Cheater::size_type sizelimit, double const parameters[2])
{
//std::cout << "Making cheat sheet of " << iter->first;
    Cheater tree(iter, typecodes, parameters);
//std::cout << " done\n";
    if (!iter->second.istrivial(iter->second.expression) &&
        !typecodes.isprimitive(iter->second.expression[0]) &&
        tree.cheatsheet().empty())
    {
        printass(*iter);
        std::cerr << "Cheat sheet generation error" << std::endl;
        return 0;
    }
//std::cout << "Cheating proof of " << iter->first << std::endl;
    tree.play(sizelimit);
    // Check answer
    if (tree.size() > sizelimit || tree.value() != 1)
        printass(*iter);
    if (tree.size() > sizelimit)
    {
        std::cerr << "Tree size limit exceeded. Main line:" << std::endl;
        tree.printmainline();
    }
    if (tree.value() != 1)
        std::cerr << "proof search test failed" << std::endl;
    return tree.value() == 1 ? tree.size() : 0;
}

// Test proof MCTS search. Return 1 iff okay.
bool testproofsearch
    (Assiters const & assiters, Typecodes const & typecodes,
     Cheater::size_type const sizelimit, double const parameters[2],
     Assiters::size_type const batch)
{
    Timer timer;
    Cheater::size_type nodecount(0);
    bool okay(true);
    // Test assertions.
    for (Assiters::size_type i(1); i < assiters.size(); ++i)
    {
        Assiter iter(assiters[i]);
        if (batch != 0 && i % batch == 0)
            printass(*iter);
        if (iter->second.proofsteps.empty())
            continue; // Skip assertions without proof.
        Cheater::size_type const n(testproofsearch(iter, typecodes,
                                                   sizelimit, parameters));
        if (n == 0)
        {
            okay = false;
            nodecount += sizelimit + 1;
            break;
        }

        if (batch != 0 && i % batch == 0)
            std::cout << n << " nodes" << std::endl;
        nodecount += n;
    }
    // Collect statistics.
    double const t(timer);
    std::cout << nodecount << " nodes / " << t << "s = ";
    std::cout << nodecount/t << " nps" << std::endl;
    return okay;
}
