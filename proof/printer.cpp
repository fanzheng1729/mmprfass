#include "../ass.h"
#include "../comment.h"
#include "../io.h"
#include "../util/arith.h"
#include "verify.h"

bool Printer::addstep(Expression const & stacktop,
                      strview label, std::size_t level, Symbol2 save)
{
    if (stacktop.empty())
        return false;

    if (ptypes->isprimitive(stacktop.front()))
        return true;

    stack.push_back(steps.size());

    steps.push_back(Expression(stacktop.size() + 3));

    Expression & dest(steps.back());
    dest[0] = label;
    dest[1].id = level;
    dest[2] = save;
    std::copy(stacktop.begin(), stacktop.end(), dest.begin() + 3);

    return true;
}

bool Printer::doaddstep(Proofstep step, Expression const & stacktop)
{
//std::cout << "Step " << step << ", top of stack: " << stacktop;
    switch (step.type)
    {
    case Proofstep::HYP:
        return addstep(stacktop, step.phyp->first);
    case Proofstep::ASS:
        {
            Assertion const & ass(step.pass->second);
            Hypsize const esscount(ass.hypcount() - ass.varcount());
            if (!enoughitemonstack(esscount, stack.size(), ""))
                return false;
            // Indent all the essential hypotheses.
            std::size_t maxlevel(0);
            if (esscount > 0)
            {
                // Find the maximal level of hypotheses.
                for (Hypsize i(0); i < esscount; ++i)
                {
                    std::size_t level(steps[stack[stack.size() - 1 - i]][1].id);
                    maxlevel = std::max(maxlevel, level);
                }
                // Promote all hypotheses to the maximal level.
                for (Hypsize i(0); i < esscount; ++i)
                {
                    steps[stack[stack.size() - 1 - i]][1].id = maxlevel;
//std::cout << stack[stack.size() - 1 - i] << '=' << maxlevel << ' ';
                }
//std::cout << std::endl;
                // Pop all the essential hypotheses.
                stack.resize(stack.size() - esscount);
            }
            return addstep(stacktop, step.pass->first,
                           (esscount > 0) * (maxlevel + 1));
        }
    case Proofstep::LOAD:
        return addstep(stacktop, "", 0, Symbol2("<=", step.index + 1));
    case Proofstep::SAVE:
        if (stacktop.empty())
            return false;
        if (!ptypes->isprimitive(stacktop[0]))
            steps.back()[2] = Symbol2(">=", ++savecount);
        return true;
    default:
        return false;
    }
    return false;
}

std::string Printer::str() const
{
    if (steps.empty())
        return "";
    std::string result;
    // Level of the conclusion
    std::size_t const maxlevel(steps.back()[1]);
    FOR (Expression const & step, steps)
    {
        // Justification + indentation + expression + tag #
        (result += strview(step[0])) += "\t";
        result += std::string(maxlevel - step[1], ' ');
        for (Expression::size_type i(3); i < step.size(); ++i)
            (result += strview(step[i])) += " ";
        if (step[2] == "<=" || step[2] == ">=")
            ((result += strview(step[2])) += " ") += util::hex(step[2].id);
        result += "\n";
    }

    return result;
}
