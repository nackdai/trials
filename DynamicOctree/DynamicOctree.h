#if !defined(__DYNAMIC_OCTREE_H__)
#define __DYNAMIC_OCTREE_H__

#include <stdint.h>
#include "izMath.h"

class DynamicOctreeObject;
class DynamicOctreeNode;

class DynamicOctree {
    friend class DynamicOctreeNode;

public:
    DynamicOctree();
    ~DynamicOctree();

public:
    void init(
        float initialSize,
        const izanagi::math::SVector4& initialPos,
        float minSize,
        uint32_t maxDepth);

    void clear();

    uint32_t add(DynamicOctreeObject* obj);

    DynamicOctreeNode* getRootNode()
    {
        return m_root;
    }

    uint32_t getMaxDepth() const
    {
        return m_maxDepth;
    }

    uint32_t getDepth() const
    {
        return m_depth;
    }

    void merge(uint32_t targetDepth);

private:
    void expand(const izanagi::math::SVector3& dir);

    uint32_t incrementDepth()
    {
        m_depth++;
        return m_depth;
    }
    uint32_t decrementDepth()
    {
        IZ_ASSERT(m_depth > 0);
        m_depth--;
        return m_depth;
    }

    uint32_t setDepthForExpand(uint32_t depth)
    {
        m_depth = (depth > m_depth ? depth : m_depth);
        return m_depth;
    }
    uint32_t setDepthForMerge(uint32_t depth)
    {
        m_depth = (depth < m_depth ? depth : m_depth);
        return m_depth;
    }

    static uint32_t getNewIdx(
        int32_t dirX,
        int32_t dirY,
        int32_t dirZ);

    using NodeDir = std::tuple < int32_t, int32_t, int32_t > ;

    static NodeDir getNodeDir(uint32_t idx);

private:
    DynamicOctreeNode* m_root{ nullptr };

    uint32_t m_maxDepth{ 0 };
    uint32_t m_depth{ 0 };
};

#endif    // #if !defined(__DYNAMIC_OCTREE_H__)