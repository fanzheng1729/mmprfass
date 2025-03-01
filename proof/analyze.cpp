#include "analyze.h"
#include "../ass.h"
#include "../util/arith.h"
#include "../database.h"
#include "../util/filter.h"
#include "../io.h"
#include "../msg.h"
#include "../util.h"
#include "verify.h"

// Subroutine for proof tree building.
// Process a proof step referencing an assertion (i.e., not a hypothesis).
// Add the corresponding step to the proof tree.
// Return true if okay.
static bool treeassertionref
    (Assertion const & assertion, std::vector<Proofsize> & stack,
     Prooftree::reference step)
{
    Hypsize const hypcount(assertion.hypcount());
    if (!enoughitemonstack(hypcount, stack.size(), ""))
        return false;

    Prooftreehyps::size_type const base(stack.size() - hypcount);
    Proofsize const index(stack.empty() ? 0 : stack.back() + 1);
    step.assign(stack.begin() + base, stack.end());
    // Remove hypotheses from stack.
    stack.erase(stack.begin() + base, stack.end());
    // Add conclusion to stack.
    stack.push_back(index);

    return true;
}

// Return the proof tree. For the step proof[i],
// Retval[i] = {index of root of arg1, index of root of arg2, ...}
// Return empty tree if not okay. Only for uncompressed proofs
Prooftree prooftree(Proofsteps const & steps)
{
    std::vector<Proofsize> stack;
    // Preallocate for efficiency
    stack.reserve(steps.size());
    Prooftree tree(steps.size());

    for (Proofsize i(0); i < steps.size(); ++i)
    {
        Proofstep::Type type(steps[i].type);
        if (type == Proofstep::HYP)
            stack.push_back(i);
        else if (type == Proofstep::ASS &&
                 treeassertionref(steps[i].pass->second, stack, tree[i]))
            continue;
        else return Prooftree();
    }

    return stack.size() == 1 ? tree : Prooftree();
}

// Iterator to a proof tree node
typedef Prooftree::const_iterator Treeiter;
// Find the beginning of the subtree with root iter.
static Treeiter subproofbegins(Treeiter iter)
{
    while (!iter->empty())
        iter -= iter->back() - iter->front() + 1;
    return iter;
}

// Return the indentations of all the proofs in a proof tree.
static void indentation(Treeiter begin, Treeiter end,
                        std::vector<Proofsize> & result)
{
    // Iterator to root of subtree
    Treeiter const back(end - 1);
    if (back->empty())
        return;
    // Index and level of root
    Proofsize const index(back->back() + 1), level(result[index]);
    for (Prooftreehyps::size_type i(0); i < back->size(); ++i)
    {
        // Index of root of sub tree
        Proofsize const subroot((*back)[i]);
        // Indent one more level.
        result[subroot] = level + 1;
        // Recurse to the subtree.
        Treeiter const newend(end - (index - subroot));
        Treeiter const newbegin(i == 0 ? begin :
                                end - (index - (*back)[i - 1]));
        indentation(newbegin, newend, result);
    }
}
std::vector<Proofsize> indentation(Prooftree const & tree)
{
    std::vector<Proofsize> result(tree.size());
    if (!tree.empty())
        indentation(tree.begin(), tree.end(), result);
    return result;
}

// Split a proof for all nodes found in splitters from the root.
// Add to subproofs and return the simplified proof.
Proof splitproof
    (Proof const & proof, Prooftree const & tree,
     // Proof steps to be split.
     Proof const & splitters,
     // (begin, end) of subproofs.
     Subprfs & subproofs)
{
    if (unexpected(proof.empty() || tree.size() != proof.size(),
                   "bad proof/tree", ""))
        return Proof();

    Proof result;
    // Stack to trace the path from root to node.
    // Elements are (node, index of tree[node]).
    std::vector<std::pair<Proofsize, Prooftreehyps::size_type> > stack;

    do
    {
        // Read the node to look at.
        Proofsize node(stack.empty() ? tree.size() - 1 :
                       tree[stack.back().first][stack.back().second]);

        if (!tree[node].empty() && util::filter(splitters)(proof[node]))
        {
            // Create a new frame to split the node.
            stack.push_back(std::make_pair(node, 0U));
            continue;
        }

        // Process the node. Find the corresponding subproof.
        Prfiter const begin(proof.begin() +
                            (subproofbegins(tree.begin() + node)-tree.begin()));
        Prfiter const end(proof.begin() + node + 1);
        // Numbering of subproof.
        Subprfs::size_type i(0);
        for ( ; i < subproofs.size(); ++i)
            if (util::equal(begin, end, subproofs[i].first, subproofs[i].second))
                break;
        if (i == subproofs.size())
            // Record the new subproof.
            subproofs.push_back(Subprf(begin, end));
        // Add the subproof.
        result.push_back(util::hex(i));

        if (stack.empty()) // No splitting performed.
            return result;
        // Move on to next node.
        while (++stack.back().second == tree[stack.back().first].size())
        {
            // All subtrees of the node have been traversed.
            // Record the node itself.
            result.push_back(proof[stack.back().first]);
            stack.pop_back();
            if (stack.empty()) break;
        }
    } while (!stack.empty());

    return result;
}

// Split the assumptions of a theorem.
// Retval[0] = conclusion. Retval[1]... = assumptions.
Subprfs splitassumptions
    (Proof const & proof, Prooftree const & tree,
     // Keyword lists: imps = implications, ands = "and" connectives
     Proof const & imps, Proof const & ands)
{
    if (unexpected(proof.empty() || tree.size() != proof.size(),
                   "bad proof/tree", ""))
        return Subprfs();
    // Reserve one expression for the conclusion.
    Subprfs result(1);
    // Split assumptions in an implication.
    // The index of the "wi" step being split.
    Proofsize wistep(proof.size() - 1);
    // Beginning of the split assumption.
    Proofsize begin(0);
    while (util::filter(imps)(proof[wistep]))
    {
        for (Prooftreehyps::size_type i(0); i + 1 < tree[wistep].size(); ++i)
        {
            // Split the assumption.
            Proof const subproof
                (proof.begin() + begin, proof.begin() + tree[wistep][i] + 1);
//std::cout << "Split subproof: " << subproof;
            // Extract the corresponding subtree.
            Prooftree subtree(subproof.size());
            for (Proofsize j(0); j < subproof.size(); ++j)
            {
                subtree[j].resize(tree[begin + j].size());
                for (Prooftreehyps::size_type k(0); k < tree[begin + j].size(); ++k)
                    subtree[j][k] = tree[begin + j][k] - begin;
            }
            // Split the assumption according to "and" connectives.
            Subprfs::size_type const oldsize(result.size());
            splitproof(subproof, subtree, ands, result);
            // Redirected newly found assumptions.
            for (Subprfs::size_type j(oldsize); j < result.size(); ++j)
            {
                result[j].first = result[j].first - subproof.begin()
                    + proof.begin() + begin;
                result[j].second = result[j].second - subproof.begin()
                    + proof.begin() + begin;
            }
            // Move on to next assumption.
            begin = tree[wistep][i] + 1;
        }
        // Move on to next "wi" step.
        --wistep;
    }
    // Store the conclusion in result[0].
    result[0] = Subprf(proof.begin() + begin, proof.begin() + wistep + 1);

    return result;
}

// Check if the rPolish of an expression matches a template.
static bool findsubstitutions
    (Stepiter expbegin, Stepiter expend, Treeiter exptree,
     Stepiter templatebegin, Stepiter templateend, Treeiter templatetree,
     Subprfsteps & result)
{
//std::cout << "Matching " << Proofsteps(expbegin, expend);
//std::cout << "Against " << Proofsteps(templatebegin, templateend);
    Proofstep expback(*(expend - 1)), templateback(*(templateend - 1));
    switch(templateback.type)
    {
    case Proofstep::HYP:
        {
            Symbol2::ID const id(templateback.id());
//std::cout << "Var " << templateback << " ID = " << id << std::endl;
            if (id == 0)
                return false;
            // Template hypothesis is floating. Check if it has been seen.
            if (result[id].second > result[id].first)
                return util::equal(expbegin, expend,
                                   result[id].first, result[id].second);
            result[id] = Subprfstep(expbegin, expend);
            return true;
        }
    case Proofstep::ASS:
//std::cout << "Ctor " << templateback << std::endl;
        if (expback != templateback)
            return false;
        // Check the children.
        {
            // Ends of both trees
            Treeiter exptreeend(expend - expbegin + exptree);
            Treeiter tmptreeend(templateend - templatebegin + templatetree);
            // Backs of both trees
            Prooftreehyps const & exptreeback(*(exptreeend - 1));
            Prooftreehyps const & tmptreeback(*(tmptreeend - 1));
//std::cout << exptreeback << std::endl << tmptreeback << std::endl;
            if (exptreeback.size() != tmptreeback.size())
                return false; // Children size mismatch
            // Match the children.
            for (Prooftreehyps::size_type i(0); i < exptreeback.size(); ++i)
            {
                Treeiter newexptree(i == 0 ? subproofbegins(exptreeend - 1) :
                                    exptreeend - 1 -
                                    (exptreeback.back() - exptreeback[i - 1]));
                Stepiter newexpbegin(expbegin + (newexptree - exptree));
                Stepiter newexpend(expend - 1 -
                                   (exptreeback.back() - exptreeback[i]));
                Treeiter newtmptree(i == 0 ? subproofbegins(tmptreeend - 1) :
                                    tmptreeend - 1 -
                                    (tmptreeback.back() - tmptreeback[i - 1]));
                Stepiter newtmpbegin(templatebegin + (newtmptree - templatetree));
                Stepiter newtmpend(templateend - 1 -
                                   (tmptreeback.back() - tmptreeback[i]));
                if (!findsubstitutions(newexpbegin, newexpend, newexptree,
                                       newtmpbegin, newtmpend, newtmptree,
                                       result))
                    return false;
            }
            return true;
        }
    default:
        return false;
    }

    return true;
}

// Check if the rPolish of an expression matches a template.
bool findsubstitutions
    (Proofsteps const & exp, Prooftree const & exptree,
     Proofsteps const & pattern, Prooftree const & patterntree,
     Subprfsteps & result)
{
    if (exp.empty() || exp.size() != exptree.size() ||
        pattern.empty() || pattern.size() != patterntree.size())
        return false;
    return findsubstitutions(exp.begin(), exp.end(), exptree.begin(),
                             pattern.begin(), pattern.end(), patterntree.begin(),
                             result);
}
