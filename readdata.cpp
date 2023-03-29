#include "ass.h"
#include "database.h"
#include "util/filter.h"
#include "getproof.h"
#include "io.h"
#include "util/progress.h"
#include "scope.h"
#include "sect.h"
#include "util/timer.h"
#include "token.h"
#include "proof/verify.h"

namespace {
class Imp
{
    Database & m_database;
    Scopes m_scopes;
    Sections & m_sections;
    Tokens & m_tokens;
public:
    Imp(Database & database, Sections & sections, Tokens & tokens) :
        m_database(database), m_scopes(), m_sections(sections), m_tokens(tokens)
        {}
private:
// Read rest of expression after its type.
// Discard tokens up to and including the terminator.
// Returns true iff okay.
    bool readexprest
        (char type, strview label, char const * terminator, Expression & exp);
// Read an expression from the token stream.
// Discard tokens up to and including the terminator.
// Returns the expression if okay. Otherwise returns empty expression.
    Expression readexpression
        (char type, strview label, char const * terminator);
// Read the proof labels of a compressed proof.
// Discard tokens up to and including the closing parenthesis.
// Returns proof steps if okay. Otherwise returns {NULL}.
    Proofsteps getprooflabels(strview label, Hypiters const & hyps);
// Read a compressed proof. Discard tokens up to and including $.
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
    int readcompressedproof
        (strview label, Hypiters const & hyps, Proofsteps & steps);
// Read a regular proof. Discard tokens up to and including $.
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
    int readregularproof(strview label, Proofsteps & steps);
// Read a proof. Discard tokens up to and including $.
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
    int readproof
        (strview label, Hypiters const & hyps, Proofsteps & steps)
    {
        if (m_tokens.front() != "(") // Regular (uncompressed proof)
            return readregularproof(label, steps);
        // Compressed proof
        m_tokens.pop();
        return readcompressedproof(label, hyps, steps);
    }
// Add a floating hypothesis. Return true iff okay.
    bool addfloatinghyp(strview label, strview type, strview var);
// Read $e statement. Return true iff okay.
    bool reade(strview label);
// Read $a statement. Return true iff okay.
    bool reada(strview label);
// Read $p statement. Return true iff okay.
    bool readp(strview label);
// Read $f statement. Return true iff okay.
    bool readf(strview label);
// Read labeled statement. Return true iff okay.
    bool readlabel(strview label);
// Read $d statement. Return true iff okay.
    bool readd();
// Read $c statement. Return true iff okay.
    bool readc();
// Read $v statement. Return true iff okay.
    bool readv();
public:
// Read tokens. Returns true iff okay.
    bool read(const char * upto);
};

// Read rest of expression after its type.
// Discard tokens up to and including the terminator.
// Returns true iff okay.
bool Imp::readexprest
    (char type, strview label, char const * terminator, Expression & exp)
{
    strview token;

    while (!m_tokens.empty() && (token = m_tokens.front()) != terminator)
    {
        m_tokens.pop();

        Hypptr phyp(m_scopes.getfloatinghyp(token));

        if (!phyp && !m_database.hasconst(token))
        {
            std::cerr << "In $" << type << " statement " << label;
            std::cerr << " token " << token.c_str;
            std::cerr << " found which is not a constant or variable in an";
            std::cerr << " active $f statement" << std::endl;
            return false;
        }

        exp.push_back(Symbol3(token, phyp ? m_database.varid(token) : 0, phyp));
    }

    if (unfinishedstat(m_tokens, "$" + type, label))
        return false;

    m_tokens.pop(); // Discard terminator token

    return true;
}

// Read an expression from the token stream.
// Discard tokens up to and including the terminator.
// Returns the expression if okay. Otherwise returns empty expression.
Expression Imp::readexpression
    (char type, strview label, char const * terminator)
{
    if (unfinishedstat(m_tokens, "$" + type, label))
        return Expression();

    strview typecode(m_tokens.front());
    if (!m_database.hasconst(typecode))
    {
        std::cerr << "First symbol in $" << type << " statement " << label
                  << " is " << typecode << " which is not a constant\n";
        return Expression();
    }

    Expression exp(1, Symbol3(typecode));

    m_tokens.pop();

    return readexprest(type, label, terminator, exp) ? exp : Expression();
}
} //anonymous namespace

// Construct an Assertion from an Expression. That is, determine the
// mandatory hypotheses and disjoint variable restrictions and the #.
// The Assertion is inserted into the assertions collection.
// Return the iterator to tha assertion.
Assertions::iterator Database::addass
    (strview label, Expression const & exp, struct Scopes const & scopes,
     bool isaxiom)
{
    Assertions::value_type value(label, Assertion());
    Assertions::iterator iter(m_assertions.insert(value).first);
    Assertion & ass(iter->second);
    ass.expression = exp;
    scopes.completeass(ass);
    ass.number = assertions().size();
    ass.type |= isaxiom * Assertion::AXIOM;
    m_assvec.push_back(iter);
    return iter;
}

namespace {
// Print error message indicating a proof referring to an inactive statement.
static void proofinactivereferr(strview token, strview label)
{
    std::cerr << "Proof of theorem " << label << " refers to " << token
              << " which is not an active statement" << std::endl;
}

static bool operator+=(Proofsteps & steps, Proofstep step)
{
    if (step.type != Proofstep::NONE) steps.push_back(step);
    return step.type;
}

// Read the proof labels of a compressed proof.
// Discard tokens up to and including the closing parenthesis.
// Returns proof steps if okay. Otherwise returns {NULL}.
Proofsteps Imp::getprooflabels(strview label, Hypiters const & hyps)
{
    Proofsteps labels(hyps.begin(), hyps.end());
//std::cout << labels.size() << " hypotheses in the theorem" << std::endl;
    strview token;
    while (!m_tokens.empty() && (token = m_tokens.front()) != ")")
    {
        m_tokens.pop();

        if (unexpected(token == label, "self-reference in proof of", label))
        {
            return Proofsteps(1, Proofstep::NONE);
        }

        if (util::filter(hyps)(token))
        {
            std::cerr << "Compressed proof of theorem " << label
                      << " has mandatory hypothesis " << token
                      << " in label list" << std::endl;
            return Proofsteps(1, Proofstep::NONE);
        }

        if (!(labels += Proofstep(token, m_database.assertions(), m_scopes)))
        {
            proofinactivereferr(token, label);
            return Proofsteps(1, Proofstep::NONE);
        }
    }

    if (unfinishedstat(m_tokens, "$p", label))
        return Proofsteps(1, Proofstep::NONE);

    // Discard closing parenthesis
    m_tokens.pop();

    return labels;
}

// Read a compressed proof. Discard tokens up to and including $.
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
int Imp::readcompressedproof
    (strview label, Hypiters const & hyps, Proofsteps & steps)
{
    // Get labels
    Proofsteps const & labels(getprooflabels(label, hyps));
    if (!labels.empty() && !labels.front())
        return 0;
//std::cout << labels.size() << " hypotheses and labels" << std::endl;

    // Get proof letters
    std::string proof;
    int const okay(getproofletters(label, m_tokens, proof));
    if (okay != 1)  // Bad or incomplete proof
        return okay;

    // Get proof numbers
    Proofnumbers const & proofnumbers(getproofnumbers(label, proof));
    if (proofnumbers.empty())
        return 0;

    // Get proof steps
    steps = compressedproofsteps(labels, proofnumbers);
    return 1;
}

// Read a regular proof. Discard tokens up to and including $.
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
int Imp::readregularproof(strview label, Proofsteps & steps)
{
    bool incomplete(false);
    strview token;

    while (!m_tokens.empty() && (token = m_tokens.front()) != "$.")
    {
        m_tokens.pop();

        if (token == "?")
        {
            incomplete = true;
            steps.push_back(Proofstep());
        }

        else if (unexpected(token == label, "self-reference in proof of", label))
        {
            return 0;
        }

        if (!(steps += Proofstep(token, m_database.assertions(), m_scopes)))
        {
            proofinactivereferr(token, label);
            return 0;
        }
    }

    if (unfinishedstat(m_tokens, "$p", label))
        return 0;

    m_tokens.pop(); // Discard $. token

    return printbadprooferr(label, steps.empty() ? 0 : incomplete ? -1 : 1);
}

// Add a floating hypothesis. Return true iff okay.
bool Imp::addfloatinghyp(strview label, strview type, strview var)
{
    // Check if there is an active hypothesis on the var ($4.2.5).
    Hypptr const phyp(m_scopes.getfloatinghyp(var));
    if (phyp)
    {
        std::cerr << "Variable " << var << " already has a floating ";
        std::cerr << "hypothesis " << phyp->first << std::endl;
        return false;
    }
    // Add the hypothesis.
    Expression exp(2);
    exp[0] = type, exp[1] = Symbol3(var, m_database.varid(var));
    Hypiter const iter(m_database.addhyp(label, exp, true));
    m_scopes.back().activehyp.push_back(iter);
    m_scopes.back().floatinghyp[var] = iter;
    // Add type code.
    const_cast<Typecodes &>(m_database.typecodes())[type];
    return true;
}

// Read $e statement. Return true iff okay.
bool Imp::reade(strview label)
{
    Expression const & exp(readexpression('e', label, "$."));
    if (exp.empty())
        return false;
    // Create new essential hypothesis
    m_scopes.back().activehyp.push_back(m_database.addhyp(label, exp, false));
    return true;
}

// Read $a statement. Return true iff okay.
bool Imp::reada(strview label)
{
    Expression const & exp(readexpression('a', label, "$."));
    if (exp.empty())
        return false;
    // Add axiom to database
    m_database.addass(label, exp, m_scopes, true);
    return true;
}

// Read $p statement. Return true iff okay.
bool Imp::readp(strview label)
{
    Expression const & newtheorem(readexpression('p', label, "$="));
    if (newtheorem.empty())
        return false;

    // Add assertion to database
    Assertions::iterator iter(m_database.addass(label, newtheorem, m_scopes));
    Assertion & ass(iter->second);

    // Now for the proof
    if (unfinishedstat(m_tokens, "$p", label))
        return false;

    // Get proof steps
    Proofsteps steps;
    int okay(readproof(label, ass.hypiters, steps));
    if (okay != 1) // Incomplete: -1 -> true, bad: 0 -> false
        return okay == -1;

    // Verify proof steps
    Expression const & exp(verifyproofsteps(steps, &*iter));
    okay = provesrightthing(label, exp, ass.expression);
    if (okay)
        ass.proofsteps = steps;

    // Mark trivial assertion, whose conclusion matches one of its hypotheses.
    ass.type |= Assertion::TRIVIAL * ass.istrivial(ass.expression);

    return okay;
}

// Print error message indicating a floating hypothesis has a bad variable.
static void floatinghyperr(strview token, strview label, int err)
{
    if (err == 0)
        return;
    static const char * msg[][3] =
    {
        {"First symbol in $f statement ", " is ", " which is not a constant"},
        {"Second symbol in $f statement ", " is ", " which is not an active variable"},
        {"Second symbol in $f statement ", " is ", " which is in two $f statements"},
        {"The $f statement ", " has an extra token ", ""}
    };
    err -= TYPENOTCONST;
    std::cerr << msg[err][0] << label << msg[err][1] << token << msg[err][2];
    std::cerr << std::endl;
}

// Read $f statement. Return true iff okay.
bool Imp::readf(strview label)
{
    if (unfinishedstat(m_tokens, "$f", label))
        return false;

    strview type(m_tokens.front());

    if (!m_database.hasconst(type))
    {
        floatinghyperr(type, label, TYPENOTCONST);
        return false;
    }

    m_tokens.pop();

    if (unfinishedstat(m_tokens, "$f", label))
        return false;

    strview variable(m_tokens.front());
    int const err(m_scopes.erraddfloatinghyp(variable));
    if (err != 0)
    {
        floatinghyperr(variable, label, err);
        return false;
    }

    m_tokens.pop();

    if (unfinishedstat(m_tokens, "$f", label))
        return false;

    if (m_tokens.front() != "$.")
    {
        floatinghyperr(m_tokens.front(), label, EXTRATOKEN);
        return false;
    }

    m_tokens.pop(); // Discard $. token

    return addfloatinghyp(label, type, variable);
}

// Print error message indicating a token used in a redefinition.
static void printtokenreuse
    (const char * reuse, strview token, const char * as)
{
    std::cerr << "Attempt to " << reuse << ' ' << token;
    if (as)
        std::cerr << ' ' << as;
    std::cerr << std::endl;
}

// Read labeled statement. Return true iff okay.
bool Imp::readlabel(strview label)
{
    const char * reuse[] = {"reuse constant", "reuse variable", "reuse label"};
    const char * as[] = {"as a label", "as a label", NULL};

    if (Database::Tokentype err = m_database.canaddcvlabel(label, Database::LABEL))
    {
        printtokenreuse(reuse[err - 1], label, as[err - 1]);
        return false;
    }

    if (unfinishedstat(m_tokens, "labeled", label))
        return false;

    strview type(m_tokens.front());
    m_tokens.pop();

    bool okay(true);
    if (type == "$p")
    {
        okay = readp(label);
    }
    else if (type == "$e")
    {
        okay = reade(label);
    }
    else if (type == "$a")
    {
        okay = reada(label);
    }
    else if (type == "$f")
    {
        okay = readf(label);
    }
    else
    {
        return !unexpected(true, "token", type.c_str); // !true == false
    }

    return okay;
}

// Read $d statement. Return true iff okay.
bool Imp::readd()
{
    Symbol2s dvars;

    strview token;

    while (!m_tokens.empty() && (token = m_tokens.front()) != "$.")
    {
        m_tokens.pop();

        if (!m_scopes.isactivevariable(token))
        {
            std::cerr << "Token " << token << ", not an active variable, "
                      << "found in a $d statement" << std::endl;
            return false;
        }

        if (!dvars.insert(Symbol2(token, m_database.varid(token))).second)
        {
            std::cerr << "$d statement mentions " << token << " twice"
                      << std::endl;
            return false;
        }
    }

    if (unfinishedstat(m_tokens, "$d", ""))
        return false;

    if (dvars.size() < 2)
    {
        std::cerr << "Not enough items in $d statement" << std::endl;
        return false;
    }

    // Record it
    m_scopes.back().disjvars.push_back(dvars);

    m_tokens.pop(); // Discard $. token

    return true;
}

static bool cvreuseprecheck
    (Database::Tokentype type, const char * reuse[3], strview token,
     Database const & db)
{
    const char * as[] = {"as a constant", "as a variable"};

    if (!ismathsymboltoken(token))
    {
        printtokenreuse("declare", token, as[type - 1]);
        return false;
    }

    if (Database::Tokentype err = db.canaddcvlabel(token, type))
    {
        printtokenreuse(reuse[err - 1], token, as[type - 1]);
        return false;
    }

    return true;
}

// Print error message indicating a constant/variable list is empty.
static bool cvlistemptyerr(char type, bool isempty)
{
    if (isempty)
        std::cerr << "Empty $" << type << " statement" << std::endl;

    return isempty;
}

// Read $c statement. Return true iff okay.
bool Imp::readc()
{
    if (!m_scopes.isouter("$c statement occurs in inner block"))
        return false;

    strview token;
    bool listempty(true);
    while (!m_tokens.empty() && (token = m_tokens.front()) != "$.")
    {
        m_tokens.pop();
        listempty = false;

        const char * reuse[] = {NULL, "redeclare variable", "reuse label"};
        if (!cvreuseprecheck(Database::CONST, reuse, token, m_database))
            return false;

        if (!m_database.addconst(token))
        {
            printtokenreuse("redeclare constant", token, NULL);
            return false;
        }
    }

    if (unfinishedstat(m_tokens, "$c", ""))
        return false;

    if (cvlistemptyerr('c', listempty))
        return false;

    m_tokens.pop(); // Discard $. token

    return true;
}

// Read $v statement. Return true iff okay.
bool Imp::readv()
{
    strview token;
    bool listempty(true);
    while (!m_tokens.empty() && (token = m_tokens.front()) != "$.")
    {
        m_tokens.pop();
        listempty = false;

        const char * reuse[] = {"redeclare constant", NULL, "reuse label"};
        if (!cvreuseprecheck(Database::VAR, reuse, token, m_database))
            return false;

        if (m_scopes.isactivevariable(token))
        {
            printtokenreuse("redeclare active variable", token, "");
            return false;
        }
        m_database.addvar(token);
        m_scopes.back().activevariables.insert(token);
    }

    if (unfinishedstat(m_tokens, "$v", ""))
        return false;

    if (cvlistemptyerr('v', listempty))
        return false;

    m_tokens.pop(); // Discard $. token

    return true;
}

// Read tokens. Returns true iff okay.
bool Imp::read(const char * const upto)
{
    std::cout << "Reading and verifying data";
    Progress progress;
    Timer timer;

    // Global scope
    m_scopes.push_back(Scope());

    // Iterator to current section
    Sections::iterator iter(m_sections.begin());
    // Iterator to the end section
    Sections::const_iterator const end
        (upto ? m_sections.find(upto) : m_sections.end());
    // # tokens to read
    Tokens::size_type const size(end == m_sections.end() ? m_tokens.size() :
                                 end->second.tokenpos);
    // Read the tokens.
    while (m_tokens.position < size)
    {
        // Add assertion # to section data
        for ( ; iter != end &&
            iter->second.tokenpos <= m_tokens.position; ++iter)
            iter->second.assnumber = m_database.assertions().size();

        strview token(m_tokens.front());
        m_tokens.pop();

        bool okay(true);

        if (islabeltoken(token))
        {
            okay = readlabel(token);
        }
        else if (token == "$d")
        {
            okay = readd();
        }
        else if (token == "${")
        {
            m_scopes.push_back(Scope());
        }
        else if (token == "$}")
        {
            if (!m_scopes.pop_back())
                return false;
        }
        else if (token == "$c")
        {
            okay = readc();
        }
        else if (token == "$v")
        {
            okay = readv();
        }
        else
        {
            return !unexpected(true, "token", token); // return false;
        }
        if (!okay)
            return false;
        progress << m_tokens.position/static_cast<double>(size);
    }

    std::cout << "done in " << timer << 's' << std::endl;
    return m_scopes.isouter("${ without corresponding $}");
}
} // anonymous namespace

// Read data from tokens. Returns true iff okay.
bool Database::read(struct Sections & sections, struct Tokens & tokens,
                    struct Comments const & comments, const char * const upto)
{
    clear();
    m_commentinfo = comments;
    return Imp(*this, sections, tokens).read(upto);
}
