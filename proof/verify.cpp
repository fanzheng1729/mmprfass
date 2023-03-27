#include "../ass.h"
#include "../database.h"
#include "../disjvars.h"
#include "../io.h"
#include "../msg.h"
#include "../util/filter.h"
#include "verify.h"

// Substitution vector
typedef std::vector<std::pair<const Symbol3 *, const Symbol3 *> > Substitutions;

// Extract proof steps from a compressed proof.
Proofsteps compressedproofsteps
    (Proofsteps const & labels, Proofnumbers const & proofnumbers)
{
    Proofsteps result(proofnumbers.size()); // Preallocate for efficiency
    Proofsize const labelcount(labels.size());

    for (Proofnumbers::size_type step(0); step < proofnumbers.size(); ++step)
    {
        Proofnumbers::value_type const number(proofnumbers[step]);
        result[step] = number == 0 ? Proofstep(Proofstep::SAVE) :
                        number <= labelcount ? labels[number - 1] :
                            Proofstep(number - labelcount - 1);
    }

    return result;
}

// Extract proof steps from a regular proof.
Proofsteps regularproofsteps
    (Proof const & proof,
     Hypotheses const & hypotheses, Assertions const & assertions)
{
    // Preallocate for efficiency
    Proofsteps result(proof.size());
    std::transform(proof.begin(), proof.end(), result.begin(),
                   Proofstep::Builder(hypotheses, assertions));
    return util::filter(result)((const char *)0) ? Proofsteps() : result;
}

static bool printinproofof(strview thlabel, bool okay = false)
{
    if (!okay)
        std::cerr << " in proof of theorem " << thlabel << std::endl;
    return okay;
}

// Check if there is enough item on the stack for hypothesis verification.
bool enoughitemonstack
    (std::size_t hypcount, std::size_t stacksize, strview label)
{
    static const char hypfound[] = " hypotheses found, ";
    bool const okay(is1stle2nd(hypcount, stacksize, hypfound, itemonstack));
    return printinproofof(label, okay);
}

void printunificationfailure
    (strview thlabel, strview reflabel, Hypothesis const & hyp,
     Expression const & dest, Expression const & stackitem)
{
    std::cout << "In step " << reflabel; printinproofof(thlabel);
    std::cout << (hyp.second ? "floating" : "essential")
              << " hypothesis " << hyp.first << "expanded to\n" << dest
              << "does not match stack item\n" << stackitem;
}

static void printdisjvarserr
    (strview var1,Expression const & exp1,strview var2,Expression const & exp2,
     Disjvars const & disjvars)
{
    std::cerr << "The substitutions\n" << var1 << ":\t" << exp1
              << var2 << ":\t" << exp2
              << "violate disjoint variable hypothesis.\n"
              << "The theorem's disjoint variable hypotheses:\n" << disjvars;
}

// Check disjoint variable hypothesis in verifying an assertion reference.
static bool checkdisjvars
    (Assertion const & theorem, Disjvars const & assdisjvars,
     Substitutions const & subst)
{
    FOR (Disjvars::const_reference var, assdisjvars)
    {
        Substitutions::value_type exp1(subst[var.first]);
        Substitutions::value_type exp2(subst[var.second]);

        if (!checkdisjvars(exp1.first, exp1.second, exp2.first, exp2.second,
                           theorem.disjvars, &theorem.varsused))
        {
            printdisjvarserr(var.first, Expression(exp1.first, exp1.second),
                             var.second, Expression(exp2.first, exp2.second),
                             theorem.disjvars);
            return false;
        }
    }

    return true;
}

// Subroutine for proof verification. Verify a proof step referencing an
// assertion (i.e., not a hypothesis).
static bool verifyassertionref
    (Assptr pthm, Assptr passref, std::vector<Expression> & stack,
     Substitutions & substitutions)
{
    strview thlabel(pthm ? pthm->first : "");
    Assertion const & assertion(passref->second);

    // Find the necessary substitutions.
    prealloc(substitutions, assertion.varsused);
    std::vector<Expression>::size_type const base = findsubstitutions
        (thlabel,passref->first,passref->second.hypiters,stack,substitutions);
    if (base == stack.size())
        return false;
//std::cout << "Substitutions" << std::endl << substitutions;

    // Verify disjoint variable conditions.
    if (pthm)
        if (!checkdisjvars(pthm->second, assertion.disjvars, substitutions))
        {
            std::cerr << "In step " << passref->first;
            return printinproofof(thlabel);
        }

    // Insert new statement onto stack.
    makesubstitution(assertion.expression, stack.back(), substitutions);
    // Remove hypotheses from stack.
    stack.erase(stack.begin() + base, stack.end() - 1);

    return true;
}

// Check if the index of a load step is within the bound.
static bool enoughsavedsteps
    (Proofstep::Index index, Proofstep::Index savedsteps, strview label)
{
    bool const okay(is1stle2nd(index + 1, savedsteps,
                               "steps needed", "steps saved"));
    return printinproofof(label, okay);
}

// Subroutine for proof verification. Verify proof steps.
Expression verifyproofsteps(Proofsteps const & steps, Assptr pthm)
{
    strview thlabel(pthm ? pthm->first : "");
//std::cout << "Verifying " << thlabel << std::endl;
    std::vector<Expression> stack, savedsteps;

    Substitutions substitutions;

    FOR (Proofstep step, steps)
    {
        switch (step.type)
        {
        case Proofstep::HYP:
//std::cout << "Pushing hypothesis: " << step.phyp->first << '\n';
            stack.push_back(step.phyp->second.first);
            break;
        case Proofstep::ASS:
//std::cout << "Applying assertion: " << step.pass->first << '\n';
            if (!verifyassertionref(pthm, step.pass, stack, substitutions))
                return Expression();
            break;
        case Proofstep::LOAD:
//std::cout << "Loading saved step " << step.index << std::endl;
            if (!enoughsavedsteps(step.index, savedsteps.size(), thlabel))
                return Expression();
            stack.push_back(savedsteps[step.index]);
            break;
        case Proofstep::SAVE:
//std::cout << "Saving step " << savedsteps.size() << std::endl;
            if (stack.empty())
            {
                std::cerr << "No step to save";
                printinproofof(thlabel);
                return Expression();
            }
            savedsteps.push_back(stack.back());
            break;
        default:
            std::cerr << "Invalid step";
            printinproofof(thlabel);
            return Expression();
        }
//std::cout << "Top of stack: " << stack.back();
    }

    if (stack.size() != 1)
    {
        std::cerr << "Proof of theorem " << thlabel << stackszerr << std::endl;
        return Expression();
    }

    return Expression(stack[0].begin(), stack[0].end());
}

// Verify a regular proof. The "proof" argument should be a non-empty sequence
// of valid labels. Return the statement the "proof" proves.
// Return the empty expression if the "proof" is invalid.
Expression verifyregularproof
    (strview label, class Database const & database,
     Proof const & proof, Hypotheses const & hypotheses)
{
    Assertions const & assertions(database.assertions());
    Assiter const iter(assertions.find(label));
    Assptr const pthm(iter == assertions.end() ? NULL : &*iter);

    Proofsteps steps(regularproofsteps(proof, hypotheses, assertions));
    if (steps.empty())
    {
        std::cout << " in regular proof of " << label << std::endl;
        return Expression();
    }

    return verifyproofsteps(steps, pthm);
}

// Return if "conclusion" == "expression" to be proved.
bool provesrightthing
    (strview label, Expression const & conclusion, Expression const & expression)
{
    if (conclusion == expression)
        return true;

    std::cerr << "Proof of theorem " << label << " proves wrong statement:\n"
              << conclusion << "instead of:\n" << expression;
    return false;
}
