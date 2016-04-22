#if !defined(__DYNAMIC_OCTREE_H__)
#define __DYNAMIC_OCTREE_H__

#include <stdint.h>
#include "izMath.h"

class DynamicOctreeObject;
class DynamicOctreeNode;

class DynamicOctree {
public:
    DynamicOctree();
    ~DynamicOctree();

public:
    void init(
        float initialSize,
        const izanagi::math::SVector4& initialPos,
        float minSize);

    void add(DynamicOctreeObject* obj);

private:
    void expand(const izanagi::math::SVector3& dir);

    static int32_t getNewIdx(
        float dirX,
        float dirY,
        float dirZ);

private:
    DynamicOctreeNode* m_root{ nullptr };
};

#endif    // #if !defined(__DYNAMIC_OCTREE_H__)