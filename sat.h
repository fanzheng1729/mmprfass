#ifndef SAT_H_INCLUDED
#define SAT_H_INCLUDED

#include "cnf.h"

class Satsolver
{
public:
// Associated SAT instance
    CNFClauses const & rcnf;
// Initialize the solver with an SAT instance.
    Satsolver(CNFClauses const & cnf) : rcnf(cnf) {}
// Return true if the SAT instance is satisfiable.
    bool sat() { return backtrack_sat(&checkmodel); }
// Map: free atoms -> truth value.
// Return the empty vector if not okay.
    Bvector truthtable(Atom const nfree);
protected:
// Reference backtracking solver
    template<class T> bool backtrack_sat(T checkmodel);
private:
    static bool checkmodel
        (CNFClauses const & cnf, CNFModel const & model, Atom)
    { return cnf.okaysofar(model); }
};

template<class T>
bool Satsolver::backtrack_sat(T checkmodel)
{
    // Initial model
    CNFModel model(rcnf.atomcount());
    // Current atom being assigned
    Atom atom(0);

    while (true)
    {
        switch (model[atom])
        {
//std::cout << "Trying atom " << atom << " = " << model[atom] << '\n';
        case CNFNONE : case CNFFALSE :
            ++model[atom];
            // Check if there is a contradiction so far.
            if (checkmodel(rcnf, model, atom))
            {
                // No contradiction yet. Move to next atom.
                if (++atom == model.size())
                {
                    // All atoms assigned
                    //std::cout << model;
                    return true;
                }
            }
            // Move to next model.
            continue;
        case CNFTRUE:
            // Un-assign the current atom.
            do
            {
                model[atom] = CNFNONE;
                if (atom == 0)
                    // All models tried
                    return false;
            } while(model[--atom] == CNFTRUE);
        }
    }
}

#endif // SAT_H_INCLUDED
