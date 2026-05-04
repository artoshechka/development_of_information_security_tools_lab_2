#ifndef NODEBOOLTREE_H
#define NODEBOOLTREE_H

class BoolEquation;

class NodeBoolTree
{
public:
    explicit NodeBoolTree(BoolEquation *equation) : lt(nullptr), rt(nullptr), eq(equation) {}
    ~NodeBoolTree() = default;

    NodeBoolTree *lt;
    NodeBoolTree *rt;
    BoolEquation *eq;
};

#endif // NODEBOOLTREE_H
