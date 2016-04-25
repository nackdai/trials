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

private:
    void expand(const izanagi::math::SVector3& dir);

    void incrementDepth()
    {
        m_depth++;
    }
    void decrementDepth()
    {
        IZ_ASSERT(m_depth > 0);
        m_depth--;
    }

    static uint32_t getNewIdx(
        int32_t dirX,
        int32_t dirY,
        int32_t dirZ);

private:
    DynamicOctreeNode* m_root{ nullptr };

    uint32_t m_maxDepth{ 0 };
    uint32_t m_depth{ 0 };
};

#endif    // #if !defined(__DYNAMIC_OCTREE_H__)