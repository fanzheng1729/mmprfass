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
    bool sat() { return backtrack_sat(&checkassignment); }
// Map: free atoms -> truth value.
// Return the empty vector if not okay.
    Bvector truthtable(Atom const nfree);
protected:
// Reference backtracking solver
    template<class T> bool backtrack_sat(T checkassignment);
private:
    static bool checkassignment
        (CNFClauses const & cnf, CNFAssignment const & assignment, Atom)
    {
        return cnf.okaysofar(assignment);
    }
};

template<class T>
bool Satsolver::backtrack_sat(T checkassignment)
{
    // Initial assignment
    CNFAssignment assignment(rcnf.atomcount());
    // Current atom being assigned
    Atom atom(0);

    while (true)
    {
        switch (assignment[atom])
        {
//std::cout << "Trying atom " << atom << " = " << assignment[atom] << '\n';
        case CNFNONE : case CNFFALSE :
            ++assignment[atom];
            // Check if there is a contradiction so far.
            if (checkassignment(rcnf, assignment, atom))
            {
                // No contradiction yet. Move to next atom.
                if (++atom == assignment.size())
                {
                    // All atoms assigned
                    //std::cout << assignment;
                    return true;
                }
            }
            // Move to next assignment.
            continue;
        case CNFTRUE:
            // Un-assign the current atom.
            do
            {
                assignment[atom] = CNFNONE;
                if (atom == 0)
                    // All assignments tried.
                    return false;
            } while(assignment[--atom] == CNFTRUE);
        }
    }
}

#endif // SAT_H_INCLUDED
