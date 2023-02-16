#ifndef BUGGY_CDCL_H_INCLUDED
#define BUGGY_CDCL_H_INCLUDED

#include "../../sat.h"

class CDCL_solver : public Satsolver
{
public:
    CDCL_solver(CNFClauses const & cnf) : Satsolver(cnf), extcnf(cnf) {}
    bool sat();
private:
// Input CNF + clauses learned
    CNFClauses extcnf;
// List of (literal * 2 + istrial, clause that unit props the literal)
    typedef std::pair<std::size_t, CNFClauses::size_type> Decision;
    static bool istrial(Decision d) { return d.first & 1; }
    struct Decision_list : std::vector<Decision>
    {
        CNFAssignment toassignment(Atom atomcount) const;
// Return last propagated decision.
        CDCL_solver::Decision_list::const_reverse_iterator lastunitprop
            (CNFClause const & clause) const;
        bool movetonext(CNFClauses::size_type badindex);
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
            second->push_back(std::make_pair((*first)[clause][index] * 2,
                                             clause));
//std::cout << "Push  " << second.back().first << "," << second.back().second << std::endl;
        }
    };
    CNFClauses::size_type learn
        (CNFClauses::size_type badindex, Decision_list const & dlist);
};

#endif // BUGGY_CDCL_H_INCLUDED
