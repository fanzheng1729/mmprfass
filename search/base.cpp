#include "base.h"
#include "io.h"

void SearchBase::printtheirnode(Nodeptr ptr) const
{
    if (!ptr) return;
    std::cout << ptr->game().attempt.pass->first << ' ';
    // Evaluation of hypotheses
    FOR (Nodeptr child, *ptr.children())
        std::cout << child->eval().first << '*' << child.size() << ' ';

    std::cout << std::endl;
}

void SearchBase::printourchildren(Nodeptr ptr) const
{
    if (!ptr) return;
    // Evaluation of attempts
    FOR (Nodeptr child, *ptr.children())
    {
        Move const & move(child->game().attempt);
        switch (move.type)
        {
        case Move::DEFER :
            std::cout << "DEFER(" << child->game().defercount() << ')';
            break;
        case Move::ASS :
            std::cout << move.pass->first;
            break;
        default :
            std::cout << "NONE";
        }
        std::cout << '=' << child->eval().first << '*' << child.size()
            << '=' << UCB(child) << '\t';
    }
    std::cout << std::endl;
}

void SearchBase::printournode(Nodeptr ptr, bool detailed) const
{
    if (!ptr) return;
    Move const & lastmove(ptr.parent()->game().attempt);
    Proofsteps const & steps(ptr->game().pgoal->first);
    strview typecode(ptr->game().typecode);
    Hypsize i(lastmove.matchhyp(steps, typecode));
    strview hyp(lastmove.pass->second.hypiters[i]->first);
    Goal goal = {&steps, typecode};
    std::cout << hyp << ' ' << ptr->eval().first << '*' << ptr.size()
              << '\t' << goal.expression();
    if (detailed)
        printourchildren(ptr);
}

void SearchBase::printmainline(Nodeptr ptr, bool detailed) const
{
    if (!ptr) return;
    std::cout << "Goal " << ptr->eval().first << ' ';
    Proofsteps const & steps(ptr->game().pgoal->first);
    Goal goal = {&steps, ptr->game().typecode};
    std::cout << goal.expression();
    if (detailed)
        printourchildren(ptr);

    size_type movecount(0), defercount(0);
    for (Nodeptr child(pickchild(ptr)); child; child = pickchild(child))
    {
        if (!child->isourturn())
        {
            switch (child->game().attempt.type)
            {
            case Move::DEFER:
                ++defercount;
                continue;
            case Move::ASS:
                std::cout << ++movecount << ".\t";
                if (defercount > 0)
                    std::cout << '(' << defercount << ") ";
                defercount = 0;
                printtheirnode(child);
                continue;
            default:
                std::cerr << "Bad move" << std::endl;
                return;
            }
        }
        else if (!child->game().pparent)
            std::cout << '\t', printournode(child, detailed);
    }
}

void SearchBase::printfulltree(Nodeptr ptr, size_type level) const
{
    if (!ptr) return;
    std::cout << std::string(level, ' ') << *ptr;

    FOR (SearchBase::Nodeptr child, *ptr.children())
        printfulltree(child, level + 1);
}

void SearchBase::printfulltree() const
{
    printfulltree(data());
    std::cout << std::string(50, '-') << std::endl;
}

void SearchBase::printstats() const
{
    static const char * const s[] = {" proven, ", " pending, ", " false"};
    std::cout << size() << " nodes, ";
    for (int i(1); i >= -1; --i)
        std::cout << countgoal(static_cast<Environ::Value>(i)) << s[1 - i];
    std::cout << std::endl;
}

SearchBase::Nodeptr SearchBase::findourchild
    (Nodeptr ptr, strview ass, size_type index) const
{
    if (!ptr)
        return ptr;

    FOR (Nodeptr child, *ptr.children())
    {
        if (child->game().attempt.pass->first != ass)
            continue;
        if (index-- == 0)
            return child;
    }
    return Nodeptr();
}

void SearchBase::navigate(bool detailed) const
{
    help();

    std::string token;
    Nodeptr ptr(data());
    while (ptr)
    {
//std::cout << ptr->isourturn() << &*ptr << std::endl;
        std::cin.clear();
        std::cin >> token;
        if (token.empty())
            continue;
        char const init(token[0]);
        if (init == 'b' || init == 'e' || init == 'q')
            return;
        if (init == 'u')
            printmainline(ptr = ptr.parent(), detailed);
        else if (init == 'h' || init == 't')
            printmainline(ptr = data(), detailed);
        else if (init == 'c')
        {
            if (!ptr->isourturn())
            {
                size_type i;
                std::cin >> i;
                Children const & children(*ptr.children());
                printmainline(ptr = children[i], detailed);
                continue;
            }
            // our turn
            Children const & children(*ptr.children());
            if (children.empty())
                break;

            std::cin >> token;
            if (token == "*")
                printmainline(ptr = children.back(), detailed);
            else
            {
                size_type i;
                std::cin >> i;
                printmainline(ptr = findourchild(ptr, token, i), detailed);
//std::cout << ptr->isourturn() << &*ptr << std::endl;
            }
        }
    }
    std::cout << std::string(50, '-') << std::endl;
}

void SearchBase::help() const
{
    std::cout <<
    "For our turn, c [label] [n] changes to the n-th node using the assertion "
    "[label]\nc * changes to the last node\n"
    "For their turn, c [n] changes to the n-th hypothesis\n"
    "u[p] goes up a level\nh[ome] or t[op] goes to the top level\n"
    "b[ye] or e[xit] or q[uit] to leave navigation\n";
}
