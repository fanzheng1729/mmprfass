#include <algorithm>
#include <limits>
#include "proof/analyze.h"
#include "ass.h"
#include "io.h"
#include "msg.h"
#include "propctor.h"
#include "util/arith.h"
#include "util/find.h"
#include "util/for.h"

std::ostream & operator<<(std::ostream & out, Propctor const & propctor)
{
    if (propctor.pdef)
        out << propctor.pdef->first << ": " << std::endl;
    else
        out << "undefined" << std::endl;
    out << "LHS = " << propctor.lhs;
    out << "RHS = " << propctor.rhs;
    out << "Truth table: " << propctor.truthtable;
    out << "CNF:" << std::endl << propctor.cnf;
    out << "argument # = " << propctor.argcount << std::endl;

    return out;
}

// Check if the data of a propositional syntax constructor is okay.
bool checkpropctor(Propctor const & propctor)
{
    if (propctor.truthtable.size() != 1u << propctor.argcount)
        return false;
    if (propctor.cnf.atomcount() != propctor.argcount + 1)
        return false;

    CNFClauses cnf(propctor.cnf);
    cnf.closeoff();
    return cnf.truthtable(propctor.argcount) == propctor.truthtable;
}

static void printpropctorerr(const char * prompt, const char * sa)
{
    std::cout << sa << '\t' << prompt << " definition" << std::endl;
}

// Check if data for all propositional syntax constructor are okay.
bool Propctors::okay(Definitions const & definitions) const
{
    FOR (const_reference propsa, *this)
    {
        const char * sa(propsa.first.c_str);
        if (!definitions.count(sa))
            printpropctorerr("no", sa);
        else if (!propsa.second.pdef)
            printpropctorerr("unused", sa);
        if (unexpected(!checkpropctor(propsa.second), "bad data for", sa))
            return false;
    }

    return true;
}

Propctors::Propctors(Definitions const & definitions)
{
    // Initialize with basic propositional connectives.
    init();
    // Add data for the definitions.
    // Records already present are overwritten if there is definition for it.
    // Records already present are preserved if there is no definition for it.
    // So any mistake in hard coded data in init() will lead to logic errors!
    FOR (Definitions::const_reference def, definitions)
        adddef(definitions, def);
}

// Initialize with primitive logical connectives.
void Propctors::init()
{
    // Truth tables for primitive logical connectives
    static const bool tt[] = {1, 0, 1, 1};
    (*this)["wi"].truthtable.assign(tt, tt + 4);
    (*this)["wn"].truthtable.assign(tt, tt + 2);
    (*this)["wtru"].truthtable.assign(1, 1);
    // Other fields
    FOR (Propctors::reference r, *this)
    {
        r.second.cnf = r.second.truthtable;
        r.second.argcount = log2(r.second.truthtable.size());
    }
}

// Add a definition. Return the iterator to the entry. Otherwise return end.
Propctors::const_iterator Propctors::adddef
    (Definitions const & definitions, Definitions::const_reference rdef)
{
    const char * salabel(rdef.first);
    Definition const & def(rdef.second);
    if (unexpected(!def.pdef, "empty definition", salabel))
        return end();
//std::cout << "Def of " << salabel << ": " << def.lhs << def.rhs;
    // # arguments of the definition
    Atom const argcount(def.lhs.size() - 1);
    Atom const maxargc(std::numeric_limits<Bvector::size_type>::digits);
    if (!is1stle2nd(argcount, maxargc, varfound, varallowed))
        return end();
    // Truth table of the definition
    Bvector truthtable(1u << argcount);
    for (Bvector::size_type i(0); i < (1u << argcount); ++i)
    {
        int const entry(calctruthvalue(definitions, def.lhs, def.rhs, i));
        if (entry == -1)
            return end();
        truthtable[i] = entry;
    }
//std::cout << "Truth table of " << salabel << " = " << truthtable;
    // Data for new propositional syntax constructor
    iterator iter(insert(value_type(salabel, Propctor())).first);
    static_cast<Definition &>(iter->second) = def;
    iter->second.truthtable = truthtable;
    iter->second.cnf = truthtable;
    iter->second.argcount = argcount;
//std::cout << "Data for syntax axiom " << salabel << ":\n" << iter->second;
    return iter;
}

// Evaluate the truth table against stack. Return true if okay.
static bool evaltruthtableonstack(Bvector const & truthtable, Bvector & stack)
{
    // # arguments
    Atom argcount(log2(truthtable.size()));
    static const char varonstack[] = " variables on stack";
    if (!is1stle2nd(argcount, stack.size(), varallowed, varonstack))
        return false;

    Bvector::size_type arg(0);
    // arguments packed in binary
    for ( ; argcount > 0; --argcount)
    {
        (arg *= 2) += stack.back();
        stack.pop_back();
    }
//std::cout << "arguments = " << arg << std::endl;
    stack.push_back(truthtable[arg]);
    return true;
}

// Evaluate *iter at arg. Return -1 if not okay.
int Propctors::calctruthvalue
    (Definitions const & definitions, Proofsteps const & lhs,
     Proofsteps const & rhs, Bvector::size_type arg)
{
    Bvector stack;
//std::cout << rhs;
    FOR (Proofstep step, rhs)
    {
        Proofsteps::const_iterator iterarg
            (std::find(lhs.begin(), lhs.end() - 1, step));
        if (iterarg != lhs.end() - 1)
        {
            stack.push_back(arg >> (iterarg - lhs.begin()) & 1);
//std::cout << step << ": arg, value = " << stack.back() << std::endl;
            continue;
        }
//std::cout << step << ": connective" << std::endl;
        // Iterator to its data
        const_iterator itersa(find(key_type(step)));
        if (itersa == end() ||
            (arg == 0 && itersa->second.pdef && !checkpropctor(itersa->second)))
        {
            // Data missing or corrupt. Compute from definition.
            Definitions::const_iterator iterdf(definitions.find(step));
            if (iterdf != definitions.end())
                itersa = adddef(definitions, *iterdf);

            if (itersa == end())
                return -1; // No found in either *this or definitions
        }

        if (!evaltruthtableonstack(itersa->second.truthtable, stack))
            return -1;
    }

    if (stack.size() != 1)
    {
        std::cerr << "Syntax proof " << rhs << stackszerr << std::endl;
        return -1;
    }

    return stack[0];
}

// Add lit <-> atom (in the positive sense) to CNF.
static void addlitatomequiv(CNFClauses & cnf, Literal lit, Atom atom)
{
    CNFClauses::size_type const n(cnf.size());
    cnf.resize(n + 2);
//std::cout << "Single literal CNF (" << lit << '<->' << atom << ") added\n";
    cnf[n].resize(2),           cnf[n + 1].resize(2);
    cnf[n][0] = lit,            cnf[n][1] = 2 * atom + 1;
    cnf[n + 1][0] = lit ^ 1,    cnf[n + 1][1] = 2 * atom;
//std::cout << cnf;
}

// Add CNF clauses from reverse Polish notation.
// # of auxiliary atoms start from atomcount.
// Return true if okay. First auxiliary atom = hyps.size()
bool Propctors::addcnffromrPolish
    (Proofsteps const & proofsteps, Hypiters const & hyps,
     CNFClauses & cnf, Atom & atomcount) const
{
    Prooftree const & tree(prooftree(proofsteps));
    if (unexpected(tree.empty(), "corrupt proof tree when adding CNF from",
                   proofsteps))
        return false;

    std::vector<Literal> literals(proofsteps.size());
    for (Proofsize i(0); i < proofsteps.size(); ++i)
    {
        const char * step(proofsteps[i]);
//std::cout << "Step " << step << ":\t";
        if (proofsteps[i].type == Proofstep::HYP)
        {
            Hypsize const hypindex(util::find(hyps, step) - hyps.begin());
            if (hypindex < hyps.size())
            {
                literals[i] = (hypindex) * 2;
//std::cout << "argument " << hypindex << std::endl;
                if (proofsteps.size() != 1)
                    continue;
                // Expression with a single variable
                addlitatomequiv(cnf, literals[i], atomcount++);
//std::cout << "New CNF:\n" << cnf;
                return true;
            }
            std::cout << "Unexpected hypothesis " << step << std::endl;
            return false;
        }
//std::cout << "operator ";
        // CNF
        const_iterator const itercnf(find(step));
        if (unexpected(itercnf == end(), "connective", step))
            return false;
        // Its arguments
        std::vector<Literal> args(tree[i].size());
        for (Prooftreehyps::size_type j(0); j < args.size(); ++j)
            args[j] = literals[tree[i][j]];
        // Add the CNF.
        cnf.append(itercnf->second.cnf, atomcount, args.data(), args.size());
        literals[i] = atomcount++ * 2;
//std::cout << literals[i] / 2 << std::endl;
    }
//std::cout << "New CNF:\n" << cnf;
    return true;
}

// Translate the hypotheses of a propositional assertion to the CNF of an SAT.
CNFClauses Propctors::hypcnf(struct Assertion const & ass, Atom & count) const
{
    Hypiters const & hyps(ass.hypiters);
    // One atom for each mandatory hypotheses (only floating ones are used)
    count = hyps.size();

    CNFClauses cnf;
    // Add hypotheses.
    for (Hypsize i(0); i < hyps.size(); ++i)
    {
//std::cout << hyps[i]->first << std::endl;
        Hypothesis const & hyp(hyps[i]->second);
        if (hyp.second)
            continue; // Skip floating hypothesis.
        // Add CNF of hypothesis.
        if (!addcnffromrPolish(ass.hypsrPolish[i], hyps, cnf, count))
            return CNFClauses();
        // Assume the hypothesis.
        cnf.closeoff();
    }
    return cnf;
}

// Translate a propositional assertion to the CNF of an SAT instance.
CNFClauses Propctors::cnf
    (struct Assertion const & ass, Proofsteps const & proofsteps) const
{
    // Add hypotheses.
    Atom count(0);
    CNFClauses cnf(hypcnf(ass, count));
    // Add conclusion.
    if (!addcnffromrPolish(proofsteps, ass.hypiters, cnf, count))
        return CNFClauses();
    // Negate conclusion.
    cnf.closeoff(true);

    return cnf;
}

// Check if an expression is valid given a propositional assertion.
bool Propctors::checkpropsat
    (struct Assertion const & ass, Proofsteps const & proofsteps) const
{
    CNFClauses const & clauses(cnf(ass, proofsteps));

    if (!clauses.sat())
        return true;

    if (&proofsteps == &ass.exprPolish)
        std::cerr << "CNF:\n" << clauses << "counter-satisfiable" << std::endl;
    return false;
}
