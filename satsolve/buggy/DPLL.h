#ifndef BUGGY_DPLL_H_INCLUDED
#define BUGGY_DPLL_H_INCLUDED

#include "../../sat.h"

class DPLL_solver : public Satsolver
{
public:
    DPLL_solver(CNFClauses const & cnf) : Satsolver(cnf) {}
    bool sat();
private:
// List of literal * 2 + istrial
    typedef std::size_t Decision;
    static bool istrial(Decision d) { return d & 1; }
    struct Decision_list : std::vector<Decision>
    {
        CNFAssignment toassignment(Atom atomcount) const;
        bool movetonext();
    };
// Call back object for unit propagation
    typedef std::pair<CNFClauses const *, Decision_list *> Callbackbase;
    struct Unitpropcallback : CNFClauses::Unitpropcallback, Callbackbase
    {
        Unitpropcallback(CNFClauses const & cnf, Decision_list & dlist) :
            Callbackbase(&cnf, &dlist) {}
        void operator()
            (CNFClauses::size_type clause, CNFClause::size_type index)
        {
            second->push_back((*first)[clause][index] * 2);
//std::cout << "Push  " << second.back() << std::endl;
        }
    };
};

#endif // BUGGY_DPLL_H_INCLUDED
