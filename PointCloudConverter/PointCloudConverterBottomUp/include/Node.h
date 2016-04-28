#if !defined(__NODE_H__)
#define __NODE_H__

#include "dynamicoctree/DynamicOctreeNode.h"
#include "../SPCDFormat.h"

struct Point {
    float pos[3];
    IZ_COLOR color;

    izanagi::math::SVector4 getCenter()
    {
        izanagi::math::SVector4 ret;
        ret.Set(pos[0], pos[1], pos[2]);
        return std::move(ret);
    }
};

class Node : public DynamicOctreeNode<Point> {
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

    SPCDHeader m_header;
};

#endif    // #if !defined(__NODE_H__)