#ifndef REFBASE_H_INCLUDED
#define REFBASE_H_INCLUDED

template<class TreeNode>
class TreeNoderefbase
{
protected:
    TreeNode * p;
public:
    TreeNoderefbase(TreeNode const & node) : p(&const_cast<TreeNode &>(node)) {}
    bool operator==(TreeNoderefbase node) { return &get() == &node.get(); }
    TreeNode const & get() const { return *p; }
    TreeNode const & parent() const { return *get().parent; }
    template<class T> void reserve(T n) { p->reserve(n); }
};

#endif // REFBASE_H_INCLUDED
