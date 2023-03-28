#include "base.h"
#include "io.h"

// Check if goal appears as the goal of ptr or its ancestors.
static bool makesloop(Goal goal, SearchBase::Nodeptr ptr)
{
    if (!ptr->isourturn())
        ptr = ptr.parent();
    for ( ; ptr; ptr = ptr.parent().parent())
    {
        Node const & node(ptr->game());
        if (!node.pparent && goal == node.goal())
            return true;
    }
    return false;
}

// Check if ptr is no easier than any children of ancestor.
static bool makesloop(SearchBase::Nodeptr ptr, SearchBase::Nodeptr ancestor)
{
    Node const & node(ptr->game());
    FOR (SearchBase::Nodeptr child, *ancestor.children())
    {
        Node const & sibling(child->game());
        if (sibling.attempt.type != Move::ASS || child->eval().first == -1)
            continue; // sibling invalid
        if (child.parent() == ptr.parent() &&
            !sibling.hypvec.empty() && sibling.hypset.empty())
            continue; // brother not checked
        if (&node != &sibling && node >= sibling)
            return true;
    }
    return false;
}

// Check if ptr duplicates upstream goals.
bool makesloop(SearchBase::Nodeptr ptr)
{
    Node const & node(ptr->game());
    if (node.attempt.type != Move::ASS)
        return false;
    // Now it is a move using an assertion.
    // Check if any of the hypotheses appears in a parent node.
    const_cast<Node &>(ptr->game()).sethyps();
    for (Hypsize i(0); i < node.attempt.hypcount(); ++i)
    {
        if (node.attempt.isfloating(i))
            continue; // Skip floating hypotheses.
        Goal goal = {&node.hypvec[i]->first, node.attempt.hyptypecode(i)};
        if (makesloop(goal, ptr.parent()))
            return true;
    }
    // Check if this node is harder than a parent node.
    SearchBase::Nodeptr ancestor(ptr.parent());
    while (true)
    {
        if (makesloop(ptr, ancestor))
            return true;
        // Move up.
        if (!ancestor.parent())
            return false;
        ancestor = ancestor.parent().parent();
    }
    return false;
}

// Format: ax-mp or DEFER(n)
static void printattempt(Node const & node)
{
    Move const & move(node.attempt);
    switch (move.type)
    {
    case Move::DEFER :
        std::cout << "DEFER(" << node.defercount() << ')';
        break;
    case Move::ASS :
        std::cout << move.pass->first;
        break;
    default :
        std::cout << "NONE";
    }
}
// Format: score*size
static void printeval(SearchBase::Nodeptr ptr)
{
    std::cout << ' ' << ptr->eval().first << '*' << ptr.size();
}
// Format: score*size=UCB
static void printeval(SearchBase::Nodeptr ptr, SearchBase const & base)
{
    printeval(ptr), std::cout << '=' << base.UCB(ptr) << '\t';
}
// Format: ax-mp score*size=UCB ...
static void printourchildren(SearchBase::Nodeptr ptr, SearchBase const & base)
{
    FOR (SearchBase::Nodeptr child, *ptr.children())
    {
        printattempt(child->game());
        printeval(child, base);
    }
    std::cout << std::endl;
}
// Format: maj score*size   |- ...
static void printournode(SearchBase::Nodeptr ptr)
{
    Move const & lastmove(ptr.parent()->game().attempt);
    Proofsteps const & steps(ptr->game().pgoal->first);
    strview typecode(ptr->game().typecode);
    Hypsize i(lastmove.matchhyp(steps, typecode));
    strview hyp(lastmove.pass->second.hypiters[i]->first);
    Goal goal = {&steps, typecode};
    std::cout << hyp, printeval(ptr), std::cout << '\t' << goal.expression();
}

// Format: DEFER(n) score*size
static void printdeferline(SearchBase::Nodeptr ptr)
{
    printattempt(ptr->game());
    if (!ptr.children()->empty())
        printeval((*ptr.children())[0]);
    std::cout << std::endl;
}
// Format: min score*size maj score*size
static void printhypsline(SearchBase::Nodeptr ptr)
{
    Move const & move(ptr->game().attempt);
    Hypsize i(0);
    FOR (SearchBase::Nodeptr child, *ptr.children())
    {
        while (move.isfloating(i)) ++i;
        std::cout << move.hypiter(i)->first;
        printeval(child);
        ++i;
    }
    std::cout << std::endl;
}
// Format: ax-mp score*size score*size
static void printtheirnode(SearchBase::Nodeptr ptr)
{
    std::cout << ptr->game().attempt.pass->first;
    FOR (SearchBase::Nodeptr child, *ptr.children())
        printeval(child);
    std::cout << std::endl;
}

// Format: Goal score |- ...
static void printgoal(SearchBase::Nodeptr ptr)
{
    std::cout << "Goal " << ptr->eval().first << ' ';
    Proofsteps const & steps(ptr->game().pgoal->first);
    Goal goal = {&steps, ptr->game().typecode};
    std::cout << goal.expression();
}

// Format:
/**
 | ax-mp score*size=UCB ...
 | DEFER(n) score*size OR
 | min score*size maj score*size
**/
// n.   ax-mp score*size score*size
//      maj score*size  |- ...
/**
 | ax-mp score*size=UCB ...
**/
void SearchBase::printmainline(Nodeptr ptr, bool detailed) const
{
    if (!ptr) return;
    printgoal(ptr);
    if (detailed)
        ptr->isourturn() ? printourchildren(ptr, *this) :
            ptr->game().attempt.type == Move::DEFER ? printdeferline(ptr) :
                printhypsline(ptr);

    size_type movecount(0), defercount(0);
    for (ptr = pickchild(ptr); ptr; ptr = pickchild(ptr))
    {
        if (!ptr->isourturn())
        {
            switch (ptr->game().attempt.type)
            {
            case Move::DEFER:
                ++defercount;
                continue;
            case Move::ASS:
                std::cout << ++movecount << ".\t";
                if (defercount > 0)
                    std::cout << '(' << defercount << ") ";
                defercount = 0;
                printtheirnode(ptr);
                continue;
            default:
                std::cerr << "Bad move" << std::endl;
                return;
            }
        }
        else if (!ptr->game().pparent)
        {
            std::cout << '\t', printournode(ptr);
            if (detailed)
                printourchildren(ptr, *this);
        }
    }
}

static void printfulltree(SearchBase::Nodeptr ptr, SearchBase::size_type level)
{
    if (!ptr) return;
    std::cout << std::string(level, ' ') << *ptr;
    FOR (SearchBase::Nodeptr child, *ptr.children())
        printfulltree(child, level + 1);
}
void SearchBase::printfulltree() const
{
    ::printfulltree(data(), 0);
    std::cout << std::string(50, '-') << std::endl;
}

void SearchBase::printstats() const
{
    static const char * const s[] = {" proven, ", " pending, ", " false"};
    std::cout << size() << " nodes, ";
    for (int i(1); i >= -1; --i)
        std::cout << countgoal(static_cast<Environ::Status>(i)) << s[1 - i];
    std::cout << std::endl;
}

static SearchBase::Nodeptr findourchild
    (SearchBase::Nodeptr ptr, strview ass, SearchBase::size_type index)
{
    if (!ptr)
        return ptr;

    FOR (SearchBase::Nodeptr child, *ptr.children())
    {
        if (child->game().attempt.pass->first != ass)
            continue;
        if (index-- == 0)
            return child;
    }
    return SearchBase::Nodeptr();
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
