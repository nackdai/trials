#if !defined(__NODE_H__)
#define __NODE_H__

#include "dynamicoctree/DynamicOctreeNode.h"

class Object : public DynamicOctreeObject {
public:
    Object();
    virtual ~Object();

public:
    virtual izanagi::math::SVector4 getCenter() override
    {
        izanagi::math::SVector4 ret;
        ret.Set(pos[0], pos[1], pos[2]);
        return std::move(ret);
    }

public:
    float pos[3];
    IZ_COLOR color;
};

class Node : public DynamicOctreeNode {
public:
    Node();
    virtual ~Node();
};

#endif    // #if !defined(__NODE_H__)