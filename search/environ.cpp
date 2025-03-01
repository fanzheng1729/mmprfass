#include "environ.h"
#include "node.h"

// Check if all hypotheses of a move are valid.
bool Environ::valid(Move const & move) const
{
    move.hypvec.assign(move.hypcount(), NULL);
    for (Hypsize i(0); i < move.hypcount(); ++i)
    {
        if (move.isfloating(i))
            continue; // Skip floating hypothesis.
        // Add the essential hypothesis as a goal.
        Proofsteps const & goal(move.hyprPolish(i));
        Goalptr goalptr(const_cast<Environ *>(this)->addgoal(goal, NEW));
        // Status of the goal
        Goalstatus & status(goalptr->second.status);
        if (status == FALSE)
            return false; // Invalid goal
        // Record the goal in the hypotheses of the move.
        move.hypvec[i] = goalptr;
        // Check if the goal is a hypothesis or already proven.
        done(goalptr, move.hyptypecode(i));
        if (status != NEW) // status == PROVEN || status == PENDING
            continue; // Valid goal
        // New goal
        if ((status = valid(goal) ? PENDING : FALSE) == FALSE)
            return false; // Invalid goal
        // Simplify hypotheses needed.
        goalptr->second.hypstotrim = hypstotrim(goalptr);
//std::cout << "added " << goalptr << " in " << this;
//std::cout << ' ' << goalptr->first << goalptr->second.hypstotrim;
    }
//std::cout << moves;
    return true;
}

// Moves generated at a given stage
Moves Environ::ourmoves(Node const & node, stage_t stage) const
{
    Assiters const & assvec(m_database.assvec());
    Prooftree const & tree(prooftree(node.goalptr->first));
    Moves moves;
//std::cout << "Finding moves for " << node << " stage " << stage << std::endl;
    Assiters::size_type const limit(std::min(assvec.size(), m_number));
    for (Assiters::size_type i(1); i < limit; ++i)
    {
        Assiter const iter(assvec[i]);
        Assertion const & ass(iter->second);
        if ((ass.type & Asstype::USELESS) || !ontopic(ass))
            continue; // Skip non propositional theorems.
        if (stage == 0 || (ass.nfreevar > 0 && stage >= ass.nfreevar))
            if (tryassertion(node.goal(), tree, iter, stage, moves))
                break; // Move closes the goal.
    }
//std::cout << "Context " << moves.size() << std::endl;
    return moves;
}

// Evaluate leaf nodes, and record the proof if proven.
Eval Environ::evalourleaf(Node const & node) const
{
    if (node.defercount == 0 && !node.goalptr->second.hypstotrim.empty())
        node.penv->addsubenv(node); // Simplify non-defer leaf by trimming hyps.
    return eval(node.penv->hypslen
                + node.goalptr->first.size()
                + node.defercount);
}

Eval Environ::evaltheirleaf(Node const & node) const
{
    if (node.attempt.type == Move::DEFER)
        return eval(hypslen + node.goalptr->first.size() + node.defercount);
//std::cout << "Evaluating " << node;
    double eval(1);
    for (Hypsize i(0); i < node.attempt.hypcount(); ++i)
    {
        if (node.attempt.isfloating(i))
            continue; // Skip floating hypothesis.
        Goalptr const goalptr(node.attempt.hypvec[i]);
        if (goalptr->second.status == PROVEN)
            continue; // Skip proven goals.
        Bvector const & hypstotrim(goalptr->second.hypstotrim);
        Proofsize const newhypslen(hypstotrim.empty() ? hypslen :
                                   m_ass.hypslen(hypstotrim));
        double const neweval(score(newhypslen + goalptr->first.size()));
        if (neweval < eval)
            eval = neweval;
    }
    if (eval == 1)
        node.writeproof();
    return Eval(eval, eval == 1);
}

// Returns a label for a collection of hypotheses.
static std::string hypslabel(Hypiters const & hypiters, Bvector const & hypstotrim)
{
    static const std::string delim("+");
    std::vector<std::string> labels;
    for (Hypsize i(0); i < hypiters.size(); ++i)
        if (!(i < hypstotrim.size() && hypstotrim[i]))
            labels.push_back(delim + std::string(hypiters[i]->first));

    std::sort(labels.begin(), labels.end());
    std::string result;
    FOR (std::string const & label, labels)
        result += label;
    return result;
}

// Add a sub environment for the node. Return true iff it is added.
bool Environ::addsubenv(Node const & node)
{
    // If not at root, forward to root.
    if (penv0 != this)
        return penv0->addsubenv(node);
    // Node's sub environment pointer
    Environ * & penv(const_cast<Environ * &>(node.penv));
    // Name of sub environment
    std::string const & label(hypslabel(penv->m_ass.hypiters,
                                        node.goalptr->second.hypstotrim));
    // Try add the sub environment.
    std::pair<Subenvs::iterator, bool> const result
        (subenvs.insert(std::pair<strview, Environ *>(label, NULL)));
    // If it already exists, set the node's sub environment pointer.
    if (!result.second)
        return penv = result.first->second, false;
    // Iterator to the sub environment
    Subenvs::iterator const enviter(result.first);
    // Add the simplified assertion.
    if (Environ * p =
        makeenv(subassertions[enviter->first] = node.penv->makeass(node)))
    {
//std::cout << node.goal().expression() << "in context " << label << std::endl;
        // Set the environment's root pointer.
        p->penv0 = this;
        // Set the node's sub environment pointer.
        penv = enviter->second = p;
        return true;
    }
    return false;
}

// Add a move with only bound substitutions.
// Return true if it has no essential hypotheses.
bool Environ::addboundmove(Move const & move, Moves & moves) const
{
    if (move.closes())
    {
        moves.assign(1, move);
        return true;
    }
    if (valid(move))
        moves.push_back(move);
    return false;
}

// Add Hypothesis-oriented moves. Return false.
bool Environ::addhypmoves(Move const & move, Moves & moves,
                          Subprfsteps const & subprfsteps) const
{
    Assertion const & thm(move.pass->second);
    // Iterate through key hypotheses i of the theorem.
    FOR (Hypsize i, thm.keyhyps)
    {
//std::cout << move.pass->first << ' ' << thm.hypiters[i]->first << ' ';
        for (Hypsize j(0); j < m_ass.hypcount(); ++j)
        {
            Hypiter const hypj(m_ass.hypiters[j]);
            if (hypj->second.second)
                continue; // Skip floating hypotheses.
            // Match hypothesis j against key hypothesis i of the theorem.
            Subprfsteps newsub(subprfsteps);
            if (findsubstitutions(m_ass.hypsrPolish[j], m_ass.hypstree[j],
                                  thm.hypsrPolish[i], thm.hypstree[i],
                                  newsub))
            {
//std::cout << hypj->first << ' ' << m_ass.hypsrPolish[j];
                // Free substitutions from the key hypothesis
                Move::Substitutions substitutions(subprfsteps.size());
                for (Subprfsteps::size_type k(1); k < newsub.size(); ++k)
                    substitutions[k] = Proofsteps(newsub[k].first,
                                                  newsub[k].second);
                Move newmove(move.pass, substitutions);
                if (valid(newmove))
                    moves.push_back(newmove);
//std::cin.get();
            }
        }
    }
    return false;
}

// Try applying the assertion, and add moves if successful.
// Return true iff a move closes the goal.
bool Environ::tryassertion
    (Goal goal, Prooftree const & tree, Assiter iter, Proofsize size,
     Moves & moves) const
{
    Assertion const & ass(iter->second);
    if (ass.expression.empty() || ass.expression[0] != goal.typecode)
        return false; // Type code mismatch
//std::cout << "Trying " << iter->first << " with " << goal.expression();
    Subprfsteps subprfsteps;
    prealloc(subprfsteps, ass.varsused);
    if (!findsubstitutions(goal.prPolish, tree, ass.exprPolish, ass.exptree,
                           subprfsteps))
        return false; // Conclusion mismatch
    // Bound substitutions
    Move::Substitutions substitutions(subprfsteps.size());
    for (Subprfsteps::size_type i(1); i < subprfsteps.size(); ++i)
        substitutions[i] = Proofsteps(subprfsteps[i].first,
                                      subprfsteps[i].second);
    // Move with all bound substitutions
    Move move(&*iter, substitutions);
    return size ? addhardmoves(iter, size, move, moves) :
        ass.nfreevar ? addhypmoves(move, moves, subprfsteps) :
            addboundmove(move, moves);
}
