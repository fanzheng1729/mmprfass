#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

#include <cstddef>
#include <deque>
#include <vector>
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
    struct Node
    {
        // Pointer to the parent
        Nodeptr parent;
        // Size of the subtree, at least 1
        size_type size;
        // The value
        T value;
        // Vector of children
        Children children;
        // Constructor. DOES NOT set size
        Node(T const & x) : value(x) {}
    };
    // Pointer to the root
    Node * m_data;
public:
    class Nodeptr
    {
        Node * m_ptr;
        Nodeptr(Node * ptr) : m_ptr(ptr) {}
        friend Tree;
        friend bool operator==(Nodeptr x, Nodeptr y){return x.m_ptr==y.m_ptr;}
        friend bool operator!=(Nodeptr x, Nodeptr y){return x.m_ptr!=y.m_ptr;}
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
            FOR (Nodeptr ptr, *children())
                ptr.m_clear();
        }
        // Change the sizes of the node and its ancestors.
        // DO NOTHING if (!*this).
        void decsize(size_type const n) const
        {
            for (Nodeptr node(*this); node; node = node.parent())
                node.m_ptr->size -= n;
        }
        void incsize(size_type const n) const
        {
            for (Nodeptr node(*this); node; node = node.parent())
                node.m_ptr->size += n;
        }
        // Add a child. Return the pointer to the child.
        // *this should != NULL.
        Nodeptr m_insert(T const & value) const
        {
            // Pointer to the child.
            Node * newptr(new Node(value));
            // Update the child.
            newptr->parent = m_ptr;
            // Update the parent.
            m_ptr->children.push_back(newptr);

            return newptr;
        }
        // Add a subtree with copying. Return pointer to the child.
        // DO NOTHING and Return NULL if (!*this || !ptr).
        Nodeptr m_insert(Nodeptr ptr) const
        {
            if (!*this || !ptr) return Nodeptr();
            // Add the root of the subtree.
            Nodeptr child(m_insert(*ptr));
            child.m_ptr->size = ptr.size();
            // Add all its children.
            child.m_insertchildren(ptr);

            return child;
        }
        // Add all children of a node.
        // *this or ptr should != NULL.
        void m_insertchildren(Nodeptr ptr) const
        {
            FOR (Nodeptr child, ptr.m_ptr->children)
                m_insert(child);
        }
        // Clear the node.
        // DO NOTHING if (!*this).
        void clear()
        {
            // Adjust sizes of ancestors.
            parent().decsize(size());
            // Clear the node.
            m_clear();
            m_ptr = NULL;
        }
    public:
        Nodeptr() : m_ptr(NULL) {}
        // Check if *this is not NULL.
        operator bool() const { return m_ptr; }
        // Return pointer to the children of a node.
        // Return NULL if (!*this).
        Children const * children() const
        { return *this ? &m_ptr->children : NULL; }
        // Return the content of a node.
        T & operator*() const { return m_ptr->value; }
        T * operator->() const{ return &m_ptr->value;}
        // Return the parent. Return NULL if (!*this).
        Nodeptr parent() const { return *this ? m_ptr->parent : Nodeptr(); }
        // Return the size. Return 0 if (!*this).
        size_type size() const { return *this ? m_ptr->size : 0; }
    }; // Class Node pointer
public:
    // Construct an empty tree.
    Tree() : m_data(NULL) {}
    // Construct a tree with 1 node.
    Tree(T const & value) : m_data(new Node(value)) { m_data->size = 1; }
    // Construct tree with copying.
    // Construct the empty tree if (!ptr).
    Tree(Nodeptr ptr)
    {
        if (!ptr)
        {
            m_data = NULL;
            return;
        }
        // ptr != NULL.
        m_data = new Node(*ptr);
        // Root node
        m_data->size = ptr.size();
        // Add children.
        data().m_insertchildren(ptr);
    }
    // Copy CTOR
    Tree(Tree const & other) { new(this) Tree(other.data()); }
    // Copy =
    // Copy the empty tree if (!ptr).
    Tree & operator=(Nodeptr ptr)
    {
        data().m_clear();
        return *(new(this) Tree(ptr));
    }
    Tree & operator=(Tree const & other)
    {
        if (data() != other.data())
            *this = other.data();
        return *this;
    }
    // Clear the tree.
    void clear()
    {
        data().m_clear();
        m_data = NULL;
    }
    ~Tree() { data().m_clear(); }
    // Return pointer to the root.
    Nodeptr data() const { return m_data; }
    // Check if the tree is empty.
    bool empty() const { return !data(); }
    // Return size of the tree.
    size_type size() const {return data().size(); }
    // Clear all the children.
    // DO NOTHING if (!*ptr).
    void clearchildren(Nodeptr ptr)
    {
        if (!ptr) return;
        // Clear all the children.
        ptr.m_clearchildren();
        ptr.m_ptr->children.clear();
        // Adjust sizes of ancestors.
        ptr.decsize(size() - 1);
    }
    // Clear a child.
    // DO NOTHING if out of range.
    void clearchild(Nodeptr ptr, size_type index)
    {
        Children const * pchildren(ptr.children());
        if (!pchildren) return;
        if (index >= pchildren->size()) return;
        Nodeptr child((*pchildren)[index]);
        if (!child) return;
        // Adjust sizes of ancestors.
        ptr.decsize(child.size());
        // Clear the child.
        child.m_clear();
        ptr.m_ptr->children.erase(ptr.m_ptr->children.begin() + index);
    }
    // Add a child. Return the pointer to the child.
    // DO NOTHING and Return NULL if ptr is NULL.
    Nodeptr insert(Nodeptr ptr, T const & value)
    {
        if (!ptr) return Nodeptr();
        // Pointer to the child.
        Nodeptr child(ptr.m_insert(value));
        // Adjust sizes of ancestors.
        ptr.incsize(child.m_ptr->size = 1);

        return child;
    }
    // Add a subtree with moving. Return pointer to the child.
    // DO NOTHING and Return NULL if either pointer is NULL.
    Nodeptr insertmove(Nodeptr ptr1, Nodeptr ptr2)
    {
        if (!ptr1 || !ptr2) return Nodeptr();
        // Update the child.
        ptr2.m_ptr->parent = ptr1.m_ptr;
        // Update the parent.
        ptr1.m_ptr->children.push_back(ptr2);
        // Adjust sizes of ancestors.
        ptr1.incsize(ptr2.size());

        return ptr2;
    }
    // Replace a subtree with moving. Return pointer to the node.
    // DO NOTHING and Return NULL if either pointer is NULL.
    Nodeptr replacemove(Nodeptr & ptr1, Nodeptr & ptr2)
    {
        if (!ptr1 || !ptr2) return Nodeptr();
        // Clear the node.
        ptr1.clear();
        // Replace the root.
        ptr1.m_ptr = ptr2.m_ptr;
        // Adjust sizes of ancestors.
        ptr1.parent().incsize(size());

        return ptr1;
    }
    // Queue for breath first search.
    typedef std::deque<Nodeptr> BFSqueue;
    // Move to next node in BFS. Return NULL if tree is exhausted.
    static Nodeptr BFSnext(BFSqueue & queue)
    {
        if (queue.empty()) return Nodeptr();
        // Next node in BFS
        Nodeptr result(queue.front());
        queue.pop_front();
        // Add its children.
        Children const * pc(result.children());
        if (!pc) return Nodeptr();
        queue.insert(queue.end(), pc->begin(), pc->end());

        return result;
    }
};

#endif // TREE_H_INCLUDED
