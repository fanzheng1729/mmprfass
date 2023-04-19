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
// # of auxiliary atoms start from natom.
// Return true if okay. First auxiliary atom = hyps.size()
bool Propctors::addclause
    (Proofsteps const & formula, Hypiters const & hyps,
     CNFClauses & cnf, Atom & natom) const
{
    Prooftree const & tree(prooftree(formula));
    if (unexpected(tree.empty(), "corrupt proof tree when adding CNF from",
                   formula))
        return false;

    std::vector<Literal> literals(formula.size());
    for (Proofsize i(0); i < formula.size(); ++i)
    {
        const char * step(formula[i]);
//std::cout << "Step " << step << ":\t";
        if (formula[i].type == Proofstep::HYP)
        {
            Hypsize const hypindex(util::find(hyps, step) - hyps.begin());
            if (hypindex < hyps.size())
            {
                literals[i] = (hypindex) * 2;
//std::cout << "argument " << hypindex << std::endl;
                if (formula.size() != 1)
                    continue;
                // Expression with a single variable
                addlitatomequiv(cnf, literals[i], natom++);
//std::cout << "New CNF:\n" << cnf;
                return true;
            }
            std::cerr << "In " << formula;
            return !unexpected(true, "hypothesis", step);
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
        cnf.append(itercnf->second.cnf, natom, args.data(), args.size());
        literals[i] = natom++ * 2;
//std::cout << literals[i] / 2 << std::endl;
    }
//std::cout << "New CNF:\n" << cnf;
    return true;
}

// Translate the hypotheses of a propositional assertion to the CNF of an SAT.
Hypscnf Propctors::hypscnf(struct Assertion const & ass, Atom & natom,
                           Bvector const & extrahyps) const
{
    Hypiters const & hyps(ass.hypiters);
    natom = hyps.size(); // One atom for each floating hypotheses

    Hypscnf result;
    CNFClauses & cnf(result.first);
    result.second.resize(hyps.size());
//std::cout << "Adding clauses for ";
    for (Hypsize i(0); i < hyps.size(); ++i)
    {
        if (!hyps[i]->second.second && !(i < extrahyps.size() && extrahyps[i]))
        {
//std::cout << hyps[i]->first << ' ' << std::endl;
            if (!addclause(ass.hypsrPolish[i], hyps, cnf, natom))
                return Hypscnf();
            // Assume the hypothesis.
            cnf.closeoff((natom - 1) * 2);
        }
        result.second[i] = cnf.size();
    }
    return result;
}

// Translate a propositional assertion to the CNF of an SAT instance.
CNFClauses Propctors::cnf
    (struct Assertion const & ass, Proofsteps const & conclusion,
     Bvector const & extrahyps) const
{
    // Add hypotheses.
    Atom natom(0);
    CNFClauses cnf(hypscnf(ass, natom, extrahyps).first);
    // Add conclusion.
    if (!addclause(conclusion, ass.hypiters, cnf, natom))
        return CNFClauses();
    // Negate conclusion.
    cnf.closeoff((natom - 1) * 2 + 1);

    return cnf;
}

// Check if an expression is valid given a propositional assertion.
bool Propctors::checkpropsat
    (struct Assertion const & ass, Proofsteps const & conclusion) const
{
    CNFClauses const & clauses(cnf(ass, conclusion));

    if (!clauses.sat())
        return true;

    if (&conclusion == &ass.exprPolish)
        std::cerr << "CNF:\n" << clauses << "counter-satisfiable" << std::endl;
    return false;
}
