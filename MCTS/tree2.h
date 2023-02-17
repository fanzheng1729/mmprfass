#ifndef TREE2_H_INCLUDED
#define TREE2_H_INCLUDED

#include <cstddef>
#include <vector>

template<class T>
class Tree2
{
public:
    typedef std::size_t size_type;
    struct Node;
    // Children of a node
    typedef std::vector<Node> Children;
    // Iterator to children
    typedef typename Children::const_iterator Childiter;
    struct Node
    {
        // The value
        T value;
        // Vector of children
        Children children;
        friend Children;
        friend Tree2;
    private:
        // Pointer to the parent
        Node * parent;
        // Size of the subtree, at least 1
        size_type size;
        // Constructor, does not set size
        Node(T const & x) : value(x) {}
        ~Node() {}
    };
private:
    // Pointer to the root
    Node * m_data;
    // Fix parent pointers of children.
    // DO NOTHING if (!ptr).
    void fixparents(Node * ptr)
    {
        if (!ptr)
            return;
        // ptr != NULL.
        for (size_type i(0); i < ptr->children.size(); ++i)
        {
            ptr->children[i].parent = ptr;
            fixparents(&ptr->children[i]);
        }
    }
    // Change the sizes of the node and its ancestors.
    // DO NOTHING if (!ptr).
    void decsize(Node * ptr, size_type const n) const
    {
        for (Node * p(ptr); p; p = p->parent)
            p->size -= n;
    }
    void incsize(Node * ptr, size_type const n) const
    {
        for (Node * p(ptr); p; p = p->parent)
            p->size += n;
    }
public:
    // Construct an empty tree.
    Tree2() : m_data(NULL) {}
    // Construct a tree with 1 node.
    Tree2(T const & value) : m_data(new Node(value)) { m_data->size = 1; }
    // Construct tree with copying.
    // Construct the empty tree if (!ptr).
    Tree2(Node const * ptr)
    {
        if (!ptr)
        {
            m_data = NULL;
            return;
        }
        // ptr != NULL.
        m_data = new Node(*ptr);
        // Fix parent pointers of children.
        fixparents(data());
    }
    // Copy CTOR
    Tree2(Tree2 const & other) { new(this) Tree2(other.data()); }
    // Copy =
    Tree2 & operator=(Tree2 const & other)
    {
        if (data() == other.data())
            return *this;
        delete data();
        return *(new(this) Tree2(other.data()));
    }
    // Clear the tree.
    void clear()
    {
        delete data();
        m_data = NULL;
    }
    ~Tree2() { delete data(); }
    // Return pointer to the root.
    Node * data() { return m_data; }
    Node const * data() const { return m_data; }
    // Check if the tree is empty.
    bool empty() const { return !data(); }
    // Return size of the tree.
    size_type size() const {return data().size(); }
    // Assign a new tree without copying. Return pointer to the root.
    // Construct the empty tree if (!ptr).
    Node * move(Node const * ptr)
    {
        delete data();
        return m_data = ptr;
    }
    // Assign a new tree with copying. Return pointer to the root.
    // Construct the empty tree if (!ptr).
    Node * copy(Node const * ptr)
    {
        delete data();
        return (new(this) Tree2(ptr))->data();
    }
    // Clear all the children.
    // DO NOTHING if (!ptr).
    void clearchildren(Node * ptr)
    {
        if (!ptr) return;
        // Clear all the children.
        ptr->children.clear();
        // Adjust sizes of ancestors.
        decsize(ptr, ptr->size - 1);
    }
    // Clear a child.
    // DO NOTHING if out of range.
    void clearchild(Node * ptr, Childiter iter)
    {
        if (!ptr) return;
        if (iter < ptr->children.begin()) return;
        if (iter >= ptr->children.end()) return;
        // Adjust sizes of ancestors.
        decsize(ptr, iter->size);
        // Clear the child.
        ptr->children.erase(iter);
    }
    // Add a child. Return the pointer to the child.
    // DO NOTHING and Return NULL if ptr is NULL.
    Node * insert(Node * ptr, T const & value)
    {
        if (!ptr) return NULL;
        // Create the child.
        Node * oldchildren(ptr->children.data());
        ptr->children.push_back(value);
        ptr->children.back().size = 1;
        // Fix the parents in case of relocation.
        if (oldchildren != ptr->children.data())
            fixparents(ptr);
        // Adjust sizes of ancestors.
        incsize(ptr, 1);

        return &ptr->children.back();
    }
    // Add a subtree with moving. Return pointer to the child.
    // DO NOTHING and Return NULL if either pointer is NULL.
    Node * insertmove(Node * ptr1, Node * ptr2);
    // Add a subtree with copying. Return pointer to the child.
    // DO NOTHING and Return NULL if either pointer is NULL.
    Node * insertcopy(Node * ptr1, Node const * ptr2)
    {
        if (!ptr1 || !ptr2) return NULL;
        // Add the subtree.
        ptr1->children.push_back(*ptr2);
        // Fix the parents.
        fixparents(ptr1);
        // Adjust sizes of ancestors.
        ptr1.incsize(ptr2->size);

        return &ptr1->children.back();
    }
    // Replace a subtree with moving. Return pointer to the node.
    // DO NOTHING and Return NULL if either pointer is NULL.
    Node * replacemove(Node * ptr1, Node * ptr2);
    // Replace a subtree with copying. Return pointer to the node.
    // DO NOTHING and Return NULL if either pointer is NULL.
    Node * replacecopy(Node * ptr1, Node const * ptr2)
    {
        if (!ptr1 || !ptr2) return NULL;
        // Clear all the children.
        clearchildren(ptr1);
        // Replace the root.
        ptr1->value = ptr2->value;
        // Add all the children.
        ptr1->children = ptr2->children;
        // Fix their parents.
        fixparents(ptr1);
        // Adjust the size.
        incsize(ptr2.size - 1);

        return ptr1;
    }
};

#endif // TREE2_H_INCLUDED
