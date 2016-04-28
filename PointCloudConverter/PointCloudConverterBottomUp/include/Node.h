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
    static std::string BasePath;

public:
    Node() {}

    Node(
        float initialSize,
        const izanagi::math::SVector4& initialPos,
        float minSize)
        : DynamicOctreeNode(initialSize, initialPos, minSize)
    {}

    virtual ~Node() {}

public:
    void flush();

    void close();

private:
    FILE* m_fp{ nullptr };
};

#endif    // #if !defined(__NODE_H__)