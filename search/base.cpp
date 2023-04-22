#include "base.h"
#include "io.h"

// Check if goal appears as the goal of ptr or its ancestors.
static bool loopsback(Goal goal, SearchBase::Nodeptr ptr)
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

// Return the only open child of a node. Return NULL if it does not exist.
SearchBase::Nodeptr onlyopenchild(SearchBase::Nodeptr ptr)
{
    if (!ptr || ptr->isourturn())
        return SearchBase::Nodeptr();

    bool hasopenchild(false);
    SearchBase::Nodeptr result;
    FOR (SearchBase::Nodeptr child, *ptr.children())
    {
        if (child->eval().first == 1)
            continue;
        // Open child found.
        if (hasopenchild)
            return SearchBase::Nodeptr(); // 2 open children
        hasopenchild = true;
        result = child;
    }

    return result;
}

// Check if ptr is no easier than any children of ancestor.
static bool loopsback(SearchBase::Nodeptr ptr, SearchBase::Nodeptr ancestor)
{
    if (ancestor->game().pparent)
        return false; // Quadratic runtime, only use when ancestor is not deferred.
    Node const & node(ptr->game());
    FOR (SearchBase::Nodeptr child, *ancestor.children())
    {
        Node const & game(child->game());
        if (game.attempt.type != Move::ASS || child->eval().first == -1)
            continue; // attempt invalid
        if (ancestor == ptr.parent())
            if ((!game.attempt.hypvec.empty() && game.hypset.empty()))
                continue; // attempt not checked
        if (&node != &game && node >= game)
            return true;
        if (SearchBase::Nodeptr cousin = onlyopenchild(child))
            if (loopsback(ptr, cousin))
                return true;
    }
    return false;
}

// Check if ptr duplicates upstream goals.
bool loopsback(SearchBase::Nodeptr ptr)
{
    Move const & move(ptr->game().attempt);
    if (move.type != Move::ASS)
        return false;
    // Now it is a move using an assertion.
    // Check if any of the hypotheses appears in a parent node.
    const_cast<Node &>(ptr->game()).sethyps();
    for (Hypsize i(0); i < move.hypcount(); ++i)
    {
        if (move.isfloating(i))
            continue; // Skip floating hypotheses.
        Goal goal = {move.hypvec[i]->first, move.hyptypecode(i)};
        if (loopsback(goal, ptr.parent()))
            return true;
    }
    // Check if this node is harder than a parent node.
    SearchBase::Nodeptr ancestor(ptr.parent());
    while (true)
    {
        if (loopsback(ptr, ancestor))
            return true;
        // Move up.
        if (!ancestor.parent())
            return false;
        ancestor = ancestor.parent().parent();
    }
    return false;
}

// Format: ax-mp[!]
static void printrefname(SearchBase::Nodeptr ptr)
{
    std::cout << ptr->game().attempt.pass->first;
    if (onlyopenchild(ptr)) std::cout << '!';
}

// Format: ax-mp[!] or DEFER(n)
static void printattempt(SearchBase::Nodeptr ptr)
{
    switch (ptr->game().attempt.type)
    {
    case Move::DEFER :
        std::cout << "DEFER(" << ptr->game().defercount() << ')';
        break;
    case Move::ASS :
        printrefname(ptr);
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
        printattempt(child);
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
    Goal goal = {steps, typecode};
    std::cout << hyp, printeval(ptr), std::cout << '\t' << goal.expression();
}

// Format: DEFER(n) score*size
static void printdeferline(SearchBase::Nodeptr ptr)
{
    printattempt(ptr);
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
// Format: ax-mp[!] score*size score*size
static void printtheirnode(SearchBase::Nodeptr ptr)
{
    std::cout << ptr->game().attempt.pass->first;
    if (onlyopenchild(ptr)) std::cout << '!';
    FOR (SearchBase::Nodeptr child, *ptr.children())
        printeval(child);
    std::cout << std::endl;
}

// Format: Goal score |- ...
static void printgoal(SearchBase::Nodeptr ptr)
{
    std::cout << "Goal " << ptr->eval().first << ' ';
    Goal goal = {ptr->game().pgoal->first, ptr->game().typecode};
    std::cout << goal.expression();
}

// Format:
/**
 | ax-mp score*size=UCB ...
 | DEFER(n) score*size OR
 | min score*size maj score*size
**/
// n.   ax-mp[!] score*size score*size
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
            std::cout << '\t';
            printournode(ptr);
        }
    }
}

// Format: n nodes, x V, y ?, z X in m contests
void SearchBase::printstats() const
{
    static const char * const s[] = {" V, ", " ?, ", " X in "};
    std::cout << size() << " nodes, ";
    for (int i(1); i >= -1; --i)
        std::cout << countgoal(i) << s[1 - i];
    std::cout << countenvs() << " contexts" << std::endl;
}

static SearchBase::Nodeptr moveup(SearchBase::Nodeptr ptr)
{
    ptr = ptr.parent();
    if (onlyopenchild(ptr))
        if (ask("Go to grandparent y/n?")[0] == 'y')
            ptr = ptr.parent();
    return ptr;
}

static SearchBase::Nodeptr gototheirchild(SearchBase::Nodeptr ptr)
{
    SearchBase::size_type i;
    std::cin >> i;
    return (*ptr.children())[i];
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

static SearchBase::Nodeptr gotoourchild(SearchBase::Nodeptr ptr)
{
    if (!ptr || ptr.children()->empty())
        return SearchBase::Nodeptr();

    std::string token;
    std::cin >> token;
    if (token == "*")
        ptr = ptr.children()->back();
    else
    {
        SearchBase::size_type i;
        std::cin >> i;
        ptr = findourchild(ptr, token, i);
    }
    if (SearchBase::Nodeptr child = onlyopenchild(ptr))
        if (ask("Go to only open child y/n?")[0] == 'y')
            ptr = child;

    return ptr;
}

void SearchBase::navigate(bool detailed) const
{
    std::cout <<
    "For our turn, c [label] [n] changes to the n-th node using the assertion "
    "[label]\nc * changes to the last node\n"
    "For their turn, c [n] changes to the n-th hypothesis\n"
    "u[p] goes up a level\nh[ome] or t[op] goes to the top level\n"
    "b[ye] or e[xit] or q[uit] to leave navigation\n";

    std::string token;
    Nodeptr ptr(data());
    while (ptr)
    {
//std::cout << ptr->isourturn() << &*ptr << std::endl;
        std::cin.clear();
        std::cin >> token;
        char const init(token[0]);
        if (init == 'b' || init == 'e' || init == 'q')
            return;
        if (init == 'u')
            printmainline(ptr = moveup(ptr), detailed);
        else if (init == 'h' || init == 't')
            printmainline(ptr = data(), detailed);
        else if (init == 'c')
            printmainline(ptr = ptr->isourturn() ? gotoourchild(ptr) :
                          gototheirchild(ptr), detailed);
    }
    std::cout << std::string(50, '-') << std::endl;
}
