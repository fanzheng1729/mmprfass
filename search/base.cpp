#include "base.h"
#include "io.h"

// Check if goal appears as the goal of ptr or its ancestors.
static bool loopsback(Goal goal, SearchBase::TreeNoderef treenode)
{
    SearchBase::TreeNoderef ournode(SearchBase::isourturn(treenode) ?
                                    treenode.get() : treenode.parent());
    while (true)
    {
        Node const & node(ournode.value().game());
        if (node.defercount == 0 && goal == node.goal())
            return true;
        if (ournode.get().parent)
            ournode = *ournode.parent().parent;
        else
            break;
    }
    return false;
}

// Return the only open child of a node. Return NULL if it does not exist.
static bool onlyopenchild(SearchBase::TreeNoderef & node)
{
    if (SearchBase::isourturn(node))
        return false;

    bool hasopenchild(false);
    FOR (SearchBase::TreeNoderef child, node.get().children)
    {
        if (SearchBase::value(child) == WIN)
            continue;
        // Open child found.
        if (hasopenchild) // 2 open children
            return node = node.parent(), false;
        // 1 open child
        node = child;
        hasopenchild = true;
    }

    return hasopenchild;
}

// Check if ptr is no easier than any children of ancestor.
static bool loopsback(SearchBase::TreeNoderef treenode,
                      SearchBase::TreeNoderef ancestor)
{
    if (ancestor.value().game().defercount > 0)
        return false; // Quadratic runtime, only use when ancestor is not deferred.
    Node const & node(treenode.value().game());
    FOR (SearchBase::TreeNoderef child, ancestor.get().children)
    {
        Node const & node2(child.value().game());
        if (node2.attempt.type != Move::ASS || SearchBase::value(child) == LOSS)
            continue; // attempt invalid
        if (ancestor == treenode.parent())
            if ((!node2.attempt.hypvec.empty() && node2.hypset.empty()))
                continue; // attempt not checked
        if (&node != &node2 && node >= node2)
            return true;
        if (onlyopenchild(child)) // Recurse into the only open child.
            if (loopsback(treenode, child))
                return true;
    }
    return false;
}

// Check if ptr duplicates upstream goals.
bool loopsback(SearchBase::TreeNoderef node)
{
    Move const & move(node.value().game().attempt);
    if (move.type != Move::ASS)
        return false;
    // Now it is a move using an assertion.
    // Check if any of the hypotheses appears in a parent node.
    const_cast<Node &>(node.value().game()).sethyps();
    for (Hypsize i(0); i < move.hypcount(); ++i)
    {
        if (move.isfloating(i))
            continue; // Skip floating hypotheses.
        Goal goal = {move.hypvec[i]->first, move.hyptypecode(i)};
        if (loopsback(goal, node.parent()))
            return true;
    }
    // Check if this node is harder than a parent node.
    SearchBase::TreeNoderef ancestor(node.parent());
    while (true)
    {
        if (loopsback(node, ancestor))
            return true;
        // Move up.
        if (!ancestor.get().parent)
            return false;
        ancestor = *ancestor.get().parent->parent;
    }
    return false;
}

// Format: ax-mp[!]
static void printrefname(SearchBase::TreeNoderef node)
{
    std::cout << node.value().game().attempt.pass->first;
    if (onlyopenchild(node)) std::cout << '!';
}

// Format: ax-mp[!] or DEFER(n)
static void printattempt(SearchBase::TreeNoderef node)
{
    switch (node.value().game().attempt.type)
    {
    case Move::DEFER :
        std::cout << "DEFER(" << node.value().game().defercount << ')';
        break;
    case Move::ASS :
        printrefname(node);
        break;
    default :
        std::cout << "NONE";
    }
}
// Format: score*size
static void printeval(SearchBase::TreeNoderef node)
{
    std::cout << ' ' << SearchBase::value(node) << '*' << node.get().size;
}
// Format: score*size=UCB
static void printeval(SearchBase::TreeNoderef node, SearchBase const & base)
{
    printeval(node); std::cout << '=' << base.UCB(node) << '\t';
}

// Format: ax-mp score*size=UCB ...
static void printourchildren(SearchBase::TreeNoderef node,
                             SearchBase const & base)
{
    FOR (SearchBase::TreeNoderef child, node.get().children)
    {
        printattempt(child);
        printeval(child, base);
    }
    std::cout << std::endl;
}
// Format: maj score*size   |- ...
static void printournode(SearchBase::TreeNoderef node)
{
    Move const & lastmove(node.parent().value.game().attempt);
    Proofsteps const & steps(node.value().game().goalptr->first);
    strview typecode(node.value().game().typecode);
    Hypsize i(lastmove.matchhyp(steps, typecode));
    strview hyp(lastmove.pass->second.hypiters[i]->first);
    Goal goal = {steps, typecode};
    std::cout << hyp; printeval(node); std::cout << '\t' << goal.expression();
}

// Format: DEFER(n) score*size
static void printdeferline(SearchBase::TreeNoderef node)
{
    printattempt(node);
    if (!node.get().children.empty())
        printeval(node.get().children[0]);
    std::cout << std::endl;
}
// Format: min score*size maj score*size
static void printhypsline(SearchBase::TreeNoderef node)
{
    Move const & move(node.value().game().attempt);
    Hypsize i(0);
    FOR (SearchBase::TreeNoderef child, node.get().children)
    {
        while (move.isfloating(i)) ++i;
        std::cout << move.hypiter(i++)->first;
        printeval(child);
    }
    std::cout << std::endl;
}

// Format: ax-mp[!] score*size score*size
static void printtheirnode(SearchBase::TreeNoderef node)
{
    printrefname(node);
    FOR (SearchBase::TreeNoderef child, node.get().children)
        printeval(child);
    std::cout << std::endl;
}

// Format: Goal score |- ...
static void printgoal(SearchBase::TreeNoderef treenode)
{
    std::cout << "Goal " << SearchBase::value(treenode) << ' ';
    Node const & node(treenode.value().game());
    Goal goal = {node.goalptr->first, node.typecode};
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
void SearchBase::printmainline(TreeNoderef node, bool detailed) const
{
    printgoal(node);
    if (detailed)
        isourturn(node) ? printourchildren(node, *this) :
            node.value().game().attempt.type == Move::DEFER ? printdeferline(node) :
                printhypsline(node);

    size_type movecount(0), defercount(0);
    while (pickchild(node))
    {
        if (!isourturn(node))
        {
            Move const & move(node.value().game().attempt);
            switch (move.type)
            {
            case Move::DEFER:
                ++defercount;
                continue;
            case Move::ASS:
                std::cout << ++movecount << ".\t";
                if (defercount > 0)
                    std::cout << '(' << defercount << ") ";
                defercount = 0;
                printtheirnode(node);
                continue;
            default:
                std::cerr << "Bad move" << move << std::endl;
                return;
            }
        }
        else if (node.value().game().defercount == 0)
        {
            std::cout << '\t';
            printournode(node);
        }
    }
}

// Format: n nodes, x V, y ?, z X in m contests
void SearchBase::printstats() const
{
    static const char * const s[] = {" V, ", " ?, ", " X in "};
    std::cout << playcount() << " plays, " << size() << " nodes, ";
    for (int i(WIN); i >= LOSS; --i)
        std::cout << countgoal(i) << s[WIN - i];
    std::cout << countenvs() << " environs" << std::endl;
}

// Move up to the parent. Return true if successful.
static bool moveup(SearchBase::TreeNoderef & node)
{
    if (!node.get().parent)
        return false;

    node = node.parent();
    SearchBase::TreeNoderef parent(node.get());
    if (onlyopenchild(parent) && askyn("Go to grandparent y/n?"))
        if (node.get().parent)
            node = node.parent();
    return true;
}

// Input the index and move to the child. Return true if successful.
static bool gototheirchild(SearchBase::TreeNoderef & node)
{
    SearchBase::size_type i;
    std::cin >> i;

    SearchBase::Children const & children(node.get().children);
    return i < children.size() ? (node = children[i], true) : false;
}

// Move to the child with given assertion and index. Return true if successful.
static bool findourchild
    (SearchBase::TreeNoderef & node, strview ass, SearchBase::size_type index)
{
    FOR (SearchBase::TreeNoderef child, node.get().children)
    {
        Move const & move(child.value().game().attempt);
        if (move.type == Move::ASS && move.pass->first == ass && index-- == 0)
            return node = child, true;
    }
    return false;
}

// Input the assertion and the index and move to the child. Return true if successful.
static bool gotoourchild(SearchBase::TreeNoderef & node)
{
    if (node.get().children.empty())
        return false;

    std::string token;
    std::cin >> token;
    if (token == "*")
        node = node.get().children.back();
    else
    {
        SearchBase::size_type i;
        std::cin >> i;
        if (!findourchild(node, token, i))
            return false;
    }
    if (onlyopenchild(node) && askyn("Go to only open child y/n?"))
        node = node.parent();
    return true;
}

void SearchBase::navigate(bool detailed) const
{
    if (empty())
        return;
    std::cout <<
    "For our turn, c [label] [n] changes to the n-th node using the assertion "
    "[label]\nc * changes to the last node\n"
    "For their turn, c [n] changes to the n-th hypothesis\n"
    "u[p] goes up a level\nh[ome] or t[op] goes to the top level\n"
    "b[ye] or e[xit] or q[uit] to leave navigation\n";

    std::string token;
    TreeNoderef node(root());
    while (true)
    {
//std::cout << isourturn(node) << ' ' << &node.value() << std::endl;
        std::cin.clear();
        std::cin >> token;
        char const init(token[0]);
        if (init == 'b' || init == 'e' || init == 'q')
            return;
        if (init == 'u')
            moveup(node);
        else if (init == 'h' || init == 't')
            node = root();
        else if (init == 'c')
            isourturn(node) ? gotoourchild(node) : gototheirchild(node);
        printmainline(node, detailed);
    }
    std::cout << std::string(50, '-') << std::endl;
}
