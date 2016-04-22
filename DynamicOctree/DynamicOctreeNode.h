#if !defined(__DYNAMIC_OCTREE_NODE_H__)
#define __DYNAMIC_OCTREE_NODE_H__

#include <stdint.h>
#include "izMath.h"

class DynamicOctreeObject {
public:
    DynamicOctreeObject() {}
    virtual ~DynamicOctreeObject() {}

public:
    virtual izanagi::math::SVector4 getCenter() = 0;
};

class AABB {
public:
    AABB() {}
    ~AABB() {}

public:
    void set(
        const izanagi::math::SVector3& size,
        const izanagi::math::SVector4& pos);

    bool isContain(const izanagi::math::SVector4& p);

    const izanagi::math::CVector3& getCenter() const
    {
        return m_center;
    }

    izanagi::math::CVector3 getSize() const
    {
        izanagi::math::CVector3 size(m_max);
        size -= m_min;
        return std::move(size);
    }

private:
    izanagi::math::CVector3 m_min;
    izanagi::math::CVector3 m_max;

    izanagi::math::CVector3 m_center;
};

class DynamicOctreeNode {
    friend class DynamicOctree;

private:
    DynamicOctreeNode(
        float initialSize,
        const izanagi::math::SVector4& initialPos,
        float minSize);

    ~DynamicOctreeNode();

public:
    const izanagi::math::CVector3& getCenter() const
    {
        return m_aabb.getCenter();
    }

    izanagi::math::CVector3 getSize() const
    {
        return m_aabb.getSize();
    }

    float getMinSize() const
    {
        m_minSize;
    }

private:
    bool add(DynamicOctreeObject* obj);

    void addInternal(DynamicOctreeObject* obj);

    void addChild(uint32_t idx, DynamicOctreeNode* child);

private:
    AABB m_aabb;

    float m_minSize{ 1.0f };

    DynamicOctreeNode* m_children[8];
    std::vector<DynamicOctreeObject*> m_objects;
};

#endif    // #if !defined(__DYNAMIC_OCTREE_NODE_H__)