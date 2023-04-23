#include "environ.h"
#include "node.h"

// Check if a goal is proven or matches a hypothesis.
// If so, record its proof. Return true iff okay.
bool Environ::done(Goalptr goalptr, strview typecode) const
{
    if (goalptr->second.status == PROVEN)
        return true; // already proven
    // Check if the goal matches a hypothesis.
    Assertion const & ass(m_assiter->second);
    Hypsize const i(ass.matchhyp(goalptr->first, typecode));
    if (i == ass.hypcount())
        return false;
    // The whose proof (1 step)
    goalptr->second.status = PROVEN;
    goalptr->second.proofsteps.assign(1, ass.hypiters[i]);
    return true;
}

// Check if all hypotheses of a move are valid.
bool Environ::valid(Move const & move) const
{
    move.hypvec.assign(move.hypcount(), NULL);
    for (Hypsize i(0); i < move.hypcount(); ++i)
    {
        if (move.isfloating(i))
            continue; // Skip floating hypothesis.
        Proofsteps const & goal(move.hyprPolish(i));
        Goalptr goalptr(((Environ &)(*this)).addgoal(goal, NEW));
        Goalstatus const status(goalptr->second.status);
        if (status == FALSE)
            return false; // goal checked to be false
        move.hypvec[i] = goalptr;
        if (done(goalptr, move.hyptypecode(i)))
            continue;
        if (status == PROVEN || status == PENDING)
        {
//std::cout << "status " << status << ' ' << goalptr << " in " << this;
//std::cout << ' ' << goalptr->first << goalptr->second.hypstotrim;
            continue; // goal checked to be true
        }
        // New goal
        const bool okay(valid(goal));
        goalptr->second.status = okay ? PENDING : FALSE;
        if (!okay) // Invalid goal found
            return false;
        goalptr->second.hypstotrim = hypstotrim(goalptr);
//std::cout << "added " << goalptr << " in " << this;
//std::cout << ' ' << goalptr->first << goalptr->second.hypstotrim;
    }
//std::cout << moves;
    return true;
}

// Moves generated at a given stage
Moves Environ::ourmoves(Node const & node, std::size_t stage) const
{
    Assiters const & assvec(m_database.assvec());
    Prooftree tree(prooftree(node.goalptr->first));
    Moves moves;
//std::cout << "Finding moves for " << node;
//std::cout << "stage " << stage << std::endl;
    for (Assiters::size_type i
         (1); i < m_assiter->second.number && i < assvec.size(); ++i)
    {
        Assiter const iter(assvec[i]);
        Assertion const & ass(iter->second);
        if ((ass.type & Assertion::USELESS) || !ontopic(ass))
            continue; // Skip non propositional theorems.
        if ((ass.nfreevar > 0) == (stage > 0) ||
            (!ass.keyhyps.empty() && stage == 0))
            if (tryassertion(node.goal(), tree, iter, stage, moves))
                break; // Move closes the goal.
    }
//std::cout << "Context " << moves.size() << std::endl;
    return moves;
}

// Evaluate leaf nodes, and record the proof if proven.
Eval Environ::evalourleaf(Node const & node) const
{
    if (!node.pparent && !node.goalptr->second.hypstotrim.empty())
        trimhyps(node); // Only for non-defer moves with hypotheses to trim
    return eval(node.penv->hypslen
                + node.goalptr->first.size()
                + node.defercount());
}
Eval Environ::evaltheirleaf(Node const & node) const
{
    if (node.attempt.type != Move::ASS)
        return eval(hypslen + node.goalptr->first.size() + node.defercount());
//std::cout << "Evaluating " << node;
    double eval(1);
    for (Hypsize i(0); i < node.attempt.hypcount(); ++i)
    {
        if (node.attempt.isfloating(i))
            continue; // Skip floating hypothesis.
        Goalptr goalptr(node.attempt.hypvec[i]);
        if (done(goalptr, node.attempt.hyptypecode(i)))
            continue; // Skip proven goals.
        Bvector const & hypstotrim(goalptr->second.hypstotrim);
        Proofsize const newhypslen(hypstotrim.empty() ? hypslen :
                                   m_assiter->second.hypslen(hypstotrim));
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
    // Node's sub environment pointer
    Environ * & penv(const_cast<Environ * &>(node.penv));
    std::string label(hypslabel(penv->m_assiter->second.hypiters,
                                node.goalptr->second.hypstotrim));
    // Find if the sub environment named label already exists.
    Subenvs::iterator enviter(subenvs.find(label));
    // If so, point the node's sub environment pointer to it.
    if (enviter != subenvs.end())
        return penv = enviter->second, false;

    enviter = subenvs.insert(std::pair<strview, Environ *>(label, NULL)).first;
    // Simplified assertion
    Assertions::value_type value(enviter->first, node.penv->makeass(node));
    // Iterator to the simplified assertion
    Assiter const assiter(subassertions.insert(value).first);
    if (Environ * p = makeenv(assiter))
    {
//std::cout << node.goal().expression() << "in context " << label << std::endl;
        // Point the node's environment to the sub environment.
        penv = enviter->second = p;
        // Prepare the sub environment.
        penv->m_assiter = assiter;
        return true;
    }
    return false;
}

// Add a move with no free variables.
// Return true iff a move closes the goal.
bool Environ::addeasymove(Move const & move, Moves & moves) const
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

// Add Hypothesis-oriented moves.
void Environ::addhypmoves(Move const & move, Moves & moves,
                          Subprfsteps const & subprfsteps) const
{
    Assertion const & thm(move.pass->second);
    Assertion const & ass(m_assiter->second);
    // Iterate through key hypotheses i of the theorem.
    FOR (Hypsize i, thm.keyhyps)
    {
        Hypiter const hypi(thm.hypiters[i]);
//std::cout << move.pass->first << ' ' << hypi->first << ' ';
        for (Hypsize j(0); j < ass.hypcount(); ++j)
        {
            Hypiter const hypj(ass.hypiters[j]);
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
                if (Environ::valid(newmove))
                    moves.push_back(newmove);
//std::cin.get();
            }
        }
    }
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
    if (size == 0)
    {
        if (ass.nfreevar == 0)
            return addeasymove(move, moves);
        // Hypothesis-oriented moves
        addhypmoves(move, moves, subprfsteps);
        return false;
    }
    // size > 0
    addhardmove(iter, size, move, moves);
    return false;
}
