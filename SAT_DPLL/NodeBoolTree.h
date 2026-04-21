#ifndef NODEBOOLTREE_H
#define NODEBOOLTREE_H

#include "Allocator.h"

class BoolEquation;

class NodeBoolTree
{
public:
    explicit NodeBoolTree(BoolEquation *equation) : lt(nullptr), rt(nullptr), eq(equation) {}
    ~NodeBoolTree() = default;

    NodeBoolTree *lt;
    NodeBoolTree *rt;
    BoolEquation *eq;

    DECLARE_ALLOCATOR
};

#endif // NODEBOOLTREE_H
