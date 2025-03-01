#ifndef TREE2_H_INCLUDED
#define TREE2_H_INCLUDED

#if __cplusplus < 201103L
#error "Must use c++11 or later"
#endif // __cplusplus

#include <cstddef>
#include <vector>
#include "refbase.h"
#include "../util/for.h"

template<class T>
class Tree2
{
public:
    typedef std::size_t size_type;
    class TreeNode;
    // Children of a node
    typedef std::vector<TreeNode> Children;
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
        friend Tree2;
        // Constructor, does not set size
        TreeNode(T const & x) : parent(NULL), value(x) {}
        // Check if the node has a grand child.
        bool hasgrandchild() const { return size > children.size() + 1; }
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
        // Fix parent pointers of children.
        void fixparents()
        {
            FOR (TreeNode & child, children)
            {
                child.parent = this;
                child.fixparents();
            }
        }
        // Clear all the children.
        void clearchildren()
        {
            children.clear();
            decsize(size - 1);
        }
        // Clear the last child.
        // DO NOTHING if no child exists.
        void pop_back()
        {
            if (children.empty()) return;
            decsize(children.back().size);
            children.pop_back();
        }
        // Clear a child.
        // DO NOTHING if index is out of range
        void clearchild(size_type index)
        {
            if (index >= children.size())
                return;
            decsize(children[index].size);
            children.erase(children.begin() + index);
            fixparents();
        }
        // Reserve space for children.
        void reserve(size_type n) { children.reserve(n); }
        // Add a child. Return the child.
        TreeNode & insert(T const & t)
        {
            bool const relocated(children.size() == children.capacity());
            children.emplace_back(t);
            children.back().size = 1;
            children.back().parent = this;
            incsize(1);
            // Fix the parents for grandchildren in case of relocation.
            if (relocated && hasgrandchild())
                FOR (TreeNode & child, children)
                    child.fixparents();

            return children.back();
        }
        // Add a subtree with copying. Return the child.
        TreeNode & insert(TreeNode const & node)
        {
            children.push_back(node);
            children.back().parent = this;
            incsize(node.size);
            // Fix the parents for grandchildren.
            if (hasgrandchild())
                FOR (TreeNode & child, children)
                    child.fixparents();

            return children.back();
        }
        template<class U>
        TreeNode & operator+=(U const & u) { return *insert(u).parent; }
        // Add a subtree with moving. Return the child.
        TreeNode & insert(TreeNode & node)
        {
            children.push_back(node.value);
            children.back().size() = node.size;
            children.back().parent = this;
            children.back().children.swap(node.children);
            incsize(node.size);
            // Fix the parents for grandchildren.
            if (hasgrandchild())
                FOR (TreeNode & child, children)
                    child.fixparents();

            return children.back();
        }
        TreeNode & operator+=(TreeNode & node) { return *insert(node).parent; }
        // Replace a subtree with copying. Return the node.
        TreeNode & operator=(T const & t)
        {
            clearchildren();
            value = t;
            return *this;
        }
        TreeNode & operator=(TreeNode const & node)
        {
            *this = node.value;
            children = node.children;
            incsize(node.size - 1);
            fixparents();

            return *this;
        }
        // Replace a subtree with moving. Return the node.
        TreeNode & operator=(TreeNode & node)
        {
            *this = node.value;
            children.swap(node.children);
            incsize(node.size - 1);
            fixparents();

            return *this;
        }
    };
private:
    // Pointer to the root
    TreeNode * m_data;
public:
    struct TreeNoderef : TreeNoderefbase<TreeNode>
    {
        friend Tree2;
        TreeNoderef(TreeNode const & node) : TreeNoderefbase<TreeNode>(node) {}
        T & value() { return p->value; }
        T const & value() const { return this->get().value; }
        TreeNoderef insert(T const & t) { return p->insert(t); }
        void pop_back() { p->pop_back(); }
    private:
        using TreeNoderefbase<TreeNode>::p;
    };
    // Construct an empty tree.
    Tree2() : m_data(NULL) {}
    // Construct a tree with 1 node.
    Tree2(T const & value) : m_data(new TreeNode(value)) { m_data->size = 1; }
    // Copy CTOR
    Tree2(Tree2 const & other) : m_data(NULL)
    {
        if (TreeNode const * p = other.m_data)
        {
            // Root node
            m_data = new TreeNode(*p);
            // Fix parent pointers of children.
            m_data->fixparents();
        }
    }
    // Copy =
    Tree2 & operator=(Tree2 const & other)
    {
        if (data() == other.data())
            return *this;
        delete data();
        return *(new(this) Tree2(other.data()));
    }
    // Clear the tree.
    void clear() { delete data(); m_data = NULL; }
    ~Tree2() { delete data(); }
    // The root node
    TreeNode const * data() const { return m_data; }
    TreeNoderef root() { return *m_data;}
    TreeNoderef const root() const { return *m_data; }
    // Check if the tree is empty.
    bool empty() const { return !data(); }
    // Return size of the tree.
    size_type size() const {return data()->size; }
    // Check data structure integrity.
    // DO NOTHING and Return true if ptr is NULL.
    bool check(TreeNode const * p) const
    {
        if (!p) return true;
        size_type n(0);
        FOR (TreeNode const & child, p->children)
        {
            if (child.parent != p)
                return false;
            n += child.size;
        }
        if (p->size != n + 1)
            return false;
        FOR (TreeNode const & child, p->children)
            if (!check(&child)) return false;
        return true;
    }
    bool check() const { return check(data()); }
};

// Return 0 -> 1 -> ... -> n.
inline Tree2<std::size_t> chain2(std::size_t n)
{
    Tree2<std::size_t> t(0);
    Tree2<std::size_t>::TreeNoderef node(t.root());
    for (std::size_t i(1); i <= n; ++i)
        node = node.insert(i);

    return t;
}

#endif // TREE2_H_INCLUDED
