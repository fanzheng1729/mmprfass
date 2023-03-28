#ifndef PRINTER_H_INCLUDED
#define PRINTER_H_INCLUDED

#include "step.h"

// Pretty printer of proof steps
struct Printer
{
    Printer(struct Typecodes const * p = NULL) : ptypes(p), savecount(0) {}
    operator bool() const { return ptypes; }
    bool addstep(Proofstep step, Expression const & stacktop)
    { return !*this || doaddstep(step, stacktop); }
    std::string str() const;
private:
    struct Typecodes const * ptypes;
    // # saved steps
    std::vector<Expression>::size_type savecount;
    // Stack of essential steps
    std::vector<std::vector<Expression>::size_type> stack;
    // List of (justification, indent level, tag #, expression)
    std::vector<Expression> steps;
    bool doaddstep(Proofstep step, Expression const & stacktop);
    bool addstep(Expression const & stacktop,
                 strview label = "", std::size_t level = 0, Symbol2 save = Symbol2());
};

#endif // PRINTER_H_INCLUDED
