#if !defined(__NODE_H__)
#define __NODE_H__

#include <atomic>
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
    friend class Writer;

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

    virtual void close() override;

    virtual bool isProcessing() const override
    {
        return m_isProcessing;
    }

private:
    void enableProcessing()
    {
        m_isProcessing = true;
    }

private:
    FILE* m_fp{ nullptr };

    std::atomic<bool> m_isProcessing{ false };

    SPCDHeader m_header;
};

#endif    // #if !defined(__NODE_H__)