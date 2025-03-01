#ifndef TREE1_H_INCLUDED
#define TREE1_H_INCLUDED

#include <cstddef>
#include <vector>
#include "refbase.h"
#include "../util/for.h"

template<class T>
class Tree
{
public:
    typedef std::size_t size_type;
    // Wrapper of pointer to a node
    class Nodeptr;
    // Children of a node
    typedef std::vector<Nodeptr> Children;
    // Iterator to children
    typedef typename Children::const_iterator Nodeptriter;
    // Wrapper of pointer to a node
private:
    // Node of the tree
    struct TreeNode
    {
        // Pointer to the parent
        TreeNode * parent;
        // Size of the subtree, at least 1
        size_type size;
        // Vector of children
        Children children;
        // The value
        T value;
        // Constructor. DOES NOT set size
        TreeNode(T const & x) : parent(NULL), value(x) {}
        // Check if the node has a grand child.
        bool hasgrandchild() const { return size > children.size() + 1; }
        // Reserve space for children.
        void reserve(size_type const n) { children.reserve(n); }
        // Change the sizes of the node and its ancestors.
        // DO NOTHING if this == NULL.
        void decsize(size_type const n)
        {
            for (TreeNode * p(this); p; p = p->parent)
                p->size -= n;
        }
        void incsize(size_type const n)
        {
            for (TreeNode * p(this); p; p = p->parent)
                p->size += n;
        }
    };
    // Pointer to the root
    TreeNode * m_data;
public:
    struct TreeNoderef : TreeNoderefbase<TreeNode>
    {
        friend Tree;
        friend Nodeptr;
        TreeNoderef(TreeNode const & node) : TreeNoderefbase<TreeNode>(node) {}
        T & value() { return p->value; }
        T const & value() const { return this->get().value; }
    private:
        using TreeNoderefbase<TreeNode>::p;
    };
    class Nodeptr
    {
        TreeNode * m_ptr;
        Nodeptr(TreeNode * ptr) : m_ptr(ptr) {}
        friend Tree;
        // Clear the node.
        // DO NOTHING if (!*this).
        void m_clear() const
        {
            if (!*this) return;
            m_clearchildren();
            delete m_ptr;
        }
        // Clear all the children.
        // *this should != NULL.
        void m_clearchildren() const
        {
            FOR (Nodeptr ptr, m_ptr->children)
                ptr.m_clear();
        }
        // Clear the node.
        // DO NOTHING if *this == NULL.
        void clear()
        {
            // Adjust sizes of ancestors.
            parent().m_ptr->decsize(size());
            // Clear the node.
            m_clear();
            m_ptr = NULL;
        }
        // Add a child. Return the pointer to the child.
        // *this should != NULL.
        TreeNoderef m_insert(T const & t) const
        {
            // Pointer to the child.
            TreeNode * newptr(new TreeNode(t));
            // Update the child.
            newptr->parent = m_ptr;
            // Update the parent.
            m_ptr->children.push_back(newptr);

            return *newptr;
        }
        // Add a subtree with copying. Return pointer to the child.
        // *this or !ptr should != NULL.
        TreeNoderef m_insert(TreeNode const & node) const
        {
            // Add the root of the subtree.
            TreeNoderef child(m_insert(node.value));
            child.p->size = node.size;
            // Add all its children.
            Nodeptr(child.p).m_insertchildren(node);

            return child;
        }
        // Add all children of a node.
        // *this or ptr should != NULL.
        void m_insertchildren(TreeNode const & node) const
        {
            FOR (Nodeptr ptr, node.children)
                m_insert(*ptr.m_ptr);
        }
    public:
        Nodeptr() : m_ptr(NULL) {}
        Nodeptr(TreeNoderef node) : m_ptr(node.p) {}
        // Check if *this != NULL.
        operator bool() const { return m_ptr; }
        // Equality check
        bool operator==(Nodeptr ptr) { return m_ptr == ptr.m_ptr; }
        bool operator!=(Nodeptr ptr) { return !(*this == ptr); }
        // Return reference to the node. *this should != NULL.
        TreeNoderef ref() const { return *m_ptr; }
        operator TreeNoderef() const { return *m_ptr; }
        // Return the parent. Return NULL if *this == NULL.
        Nodeptr parent() const { return *this ? m_ptr->parent : Nodeptr(); }
        // Return the size. Return 0 if *this == NULL.
        size_type size() const { return *this ? m_ptr->size : 0; }
        // Return the content of a node. *this != NULL.
        T & operator*() const { return m_ptr->value; }
        T * operator->() const{ return &m_ptr->value;}
        // Return pointer to the children of a node.
        // Return NULL if *this == NULL.
        Children const * children() const
        { return *this ? &m_ptr->children : NULL; }
    }; // Class Node pointer
public:
    // Construct an empty tree.
    Tree() : m_data(NULL) {}
    // Construct a tree with 1 node.
    Tree(T const & value) : m_data(new TreeNode(value)) { m_data->size = 1; }
    // Copy CTOR
    Tree(Tree const & other) : m_data(NULL)
    {
        if (TreeNode const * p = other.m_data)
        {
            // Root node
            m_data = new TreeNode(p->value);
            m_data->size = p->size;
            // Add children.
            data().m_insertchildren(*p);
        }
    }
    // Copy =
    Tree & operator=(Tree const & other)
    {
        if (data() == other.data())
            return *this;
        data().m_clear();
        return *(new(this) Tree(other.data()));
    }
    // Clear the tree.
    void clear() { data().m_clear(); m_data = NULL; }
    ~Tree() { data().m_clear(); }
    // The root node
    Nodeptr data() const { return m_data; }
    TreeNoderef root() { return *m_data;}
    TreeNoderef const root() const { return *m_data; }
    // Check if the tree is empty.
    bool empty() const { return !m_data; }
    // Return size of the tree.
    size_type size() const {return m_data->size; }
    // Clear all the children.
    void clearchildren(TreeNoderef node)
    {
        // Clear all the children.
        Nodeptr(node.p).m_clearchildren();
        node.p->children.clear();
        // Adjust sizes of ancestors.
        node.p->decsize(node.p.size - 1);
    }
    // Clear the last child.
    // DO NOTHING if no child exists.
    void pop_back(TreeNoderef node)
    {
        Children & children(node.p->children);
        if (children.empty()) return;
        // Clear the last child.
        children.back().clear();
        children.pop_back();
    }
    // Clear a child.
    // DO NOTHING if node has a grandchild or index is out of range.
    void clearchild(TreeNoderef node, size_type index)
    {
        if (node.p->hasgrandchild()) return;
        // If index is out of range, return.
        Children & children(node.p->children);
        if (index >= children.size()) return;
        // Clear the child.
        children[index].clear();
        children.erase(children.begin() + index);
    }
    // Add a child. Return the pointer to the child.
    // DO NOTHING and Return NULL if ptr is NULL.
    Nodeptr insert(Nodeptr ptr, T const & value)
    {
        // Forbid inserting if any child has a grandchild.
        if (!ptr || ptr.m_ptr->hasgrandchild()) return Nodeptr();
        // Pointer to the child.
        Nodeptr child(ptr.m_insert(value).p);
        // Adjust sizes of ancestors.
        ptr.m_ptr->incsize(child.m_ptr->size = 1);

        return child;
    }
    // Check data structure integrity.
    // DO NOTHING and Return true if ptr is NULL.
    bool check(Nodeptr ptr) const
    {
        if (!ptr) return true;
        size_type n(0);
        FOR (Nodeptr child, *ptr.children())
        {
            if (child.parent() != ptr)
                return false;
            n += child.size();
        }
        if (ptr.size() != n + 1)
            return false;
        FOR (Nodeptr child, *ptr.children())
            if (!check(child)) return false;
        return true;
    }
    bool check() const { return check(data()); }
};

// Return 0 -> 1 -> ... -> n.
inline Tree<std::size_t> chain1(std::size_t n)
{
    Tree<std::size_t> t(0);
    Tree<std::size_t>::Nodeptr p(t.data());
    for (std::size_t i(1); i <= n; ++i)
        p = t.insert(p, i);

    return t;
}

#endif // TREE1_H_INCLUDED
