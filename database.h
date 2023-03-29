#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#include "ass.h"
#include "comment.h"
#include "def.h"
#include "propctor.h"
#include "stat.h"
#include "syntaxiom.h"
#include "util/for.h"

class Database
{
    Constants m_constants;
    // Map: var -> id
    VarIDmap m_varIDmap;
    std::vector<strview> m_varvec;
    Hypotheses m_hypotheses;
    Assertions m_assertions;
    Assiters m_assvec;
    Syntaxioms m_syntaxioms;
    Commentinfo m_commentinfo;
    Equalities m_equalities;
    Definitions m_definitions;
    Propctors m_propctors;
public:
    Database() : m_assvec(1) { addvar(""); }
    bool read(struct Sections & sections, struct Tokens & tokens,
              struct Comments const & comments, const char * const upto);
    void clear() { this->~Database(); new(this) Database; }
    Symbol2::ID varid(strview str) const { return varIDmap().at(str); }
    VarIDmap const & varIDmap() const { return m_varIDmap; }
    std::vector<strview> const & varvec() const { return m_varvec; }
    Hypotheses const & hypotheses() const { return m_hypotheses; }
    Assertions const & assertions() const { return m_assertions; }
    Assiters const & assvec() const { return m_assvec; }
    Syntaxioms const & syntaxioms() const { return m_syntaxioms; }
    Syntaxioms primitivesyntaxioms() const
    {
        Syntaxioms result;
        FOR (Syntaxioms::const_reference syntaxiom, syntaxioms())
            if (!definitions().count(syntaxiom.first.c_str))
                result.insert(syntaxiom);
        return result;
    }
    Commentinfo const & commentinfo() const { return m_commentinfo; }
    Ctordefns const & ctordefns() const { return commentinfo().ctordefns; }
    Typecodes const & typecodes() const { return commentinfo().typecodes; }
    Equalities const & equalities() const { return m_equalities; }
    Definitions const & definitions() const { return m_definitions; }
    Propctors const & propctors() const { return m_propctors; }
    bool hasconst(strview str) const { return m_constants.count(str); }
    bool addconst(strview str) { return m_constants.insert(str).second; }
    bool hasvar(strview str) const { return varIDmap().count(str); }
    bool addvar(strview str)
    {
        VarIDmap::value_type value(str, varIDmap().size());
        bool const okay(m_varIDmap.insert(value).second);
        if (okay)
            m_varvec.push_back(str);
        return okay;
    }
    bool hashyp(strview label) const { return hypotheses().count(label); }
    Hypiter addhyp(strview label, Expression const & exp, bool const floating)
    {
        Hypotheses::value_type value(label, Hypothesis());
        Hypotheses::iterator result(m_hypotheses.insert(value).first);
        result->second.first = exp;
        result->second.second = floating;
        if (floating)
            result->second.first[1].phyp = &*result;
        return result;
    }
    bool hasass(strview label) const { return assertions().count(label); }
    // Construct an Assertion from an Expression. That is, determine the
    // mandatory hypotheses and disjoint variable restrictions and the #.
    // The Assertion is inserted into the assertions collection.
    // Return the iterator to tha assertion.
    Assertions::iterator addass
        (strview label, Expression const & exp, struct Scopes const & scopes,
         bool isaxiom = false);
    // Types of tokens
    enum Tokentype {OKAY, CONST, VAR, LABEL};
    // Determine if a new constant, variable or label can be added
    // If type is CONST or VAR, the corresponding type is not checked
    Tokentype canaddcvlabel(strview token, int const type) const
    {
        if (type != CONST && hasconst(token))
            return CONST;
        if (type != VAR && hasvar(token))
            return VAR;
        if (hashyp(token) || hasass(token))
            return LABEL;
        return OKAY;
    }
// Add the revPolish notation of to the whole database. Return true iff okay.
    bool rPolish()
    {
        m_syntaxioms.~Syntaxioms();
        new(&m_syntaxioms) Syntaxioms(assertions(), *this);
        bool const okay(syntaxioms().rPolish(m_assertions, typecodes()));
        if (okay)
            m_equalities = ::equalities(assertions());
        return okay;
    }
// Test syntax parser. Return 1 iff okay.
    bool checkrPolish() const;
// Find definitions in assertions.
    void loaddefinitions()
    {
        m_definitions = Definitions(assertions(), commentinfo(), equalities());
    }
// Load data related to propositional calculus.
    void loadpropasinfos() { m_propctors = definitions(); }
// Mark propositional assertions. Return its number.
    Assertions::size_type markpropassertions()
    {
        Assertions::size_type count(0);
        FOR (Assertions::reference r, m_assertions)
        {
            Assertion & ass(r.second);
            bool is(largestsymboldefnumber(ass,propctors(),Syntaxioms(),1));
            ass.type ^= is * Assertion::PROPOSITIONAL;
            if (is && !ass.expression.empty() &&
                !typecodes().isprimitive(ass.expression[0]))
                ++count;
        }
        return count;
    }
// Check if all the definitions are syntactically okay.
    bool checkdefinitions() const;
// Check if all propositional assertions are sound.
    bool checkpropassertion() const;
// Print all hard assertions.
    void printhardassertions() const;
};

#endif // DATABASE_H_INCLUDED
