#if !defined(__DYNAMIC_OCTREE_NODE_H__)
#define __DYNAMIC_OCTREE_NODE_H__

#include <stdint.h>
#include "izMath.h"

class DynamicOctree;

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

    izanagi::math::CVector3 getCenter() const;

    izanagi::math::CVector3 getSize() const
    {
        izanagi::math::CVector3 size(m_max);
        size -= m_min;
        return std::move(size);
    }

    const izanagi::math::CVector3& getMin() const
    {
        return m_min;
    }

    const izanagi::math::CVector3& getMax() const
    {
        return m_max;
    }

private:
    izanagi::math::CVector3 m_min;
    izanagi::math::CVector3 m_max;
};

class DynamicOctreeNode {
    friend class DynamicOctree;

    static uint32_t s_maxRegisteredObjCount;

    enum AddResult {
        None,
        Success,
        NotContain,
        OverFlow,
    };

public:
    static void setMaxRegisteredObjCount(uint32_t cnt)
    {
        s_maxRegisteredObjCount = cnt;
    }
    static uint32_t getMaxRegisteredObjCount()
    {
        return s_maxRegisteredObjCount;
    }

private:
    DynamicOctreeNode(
        float initialSize,
        const izanagi::math::SVector4& initialPos,
        float minSize);

    ~DynamicOctreeNode();

public:
    izanagi::math::CVector3 getCenter() const
    {
        auto center = m_aabb.getCenter();
        return std::move(center);
    }

    const izanagi::math::CVector3& getMin() const
    {
        return m_aabb.getMin();
    }

    izanagi::math::CVector3 getSize() const
    {
        return m_aabb.getSize();
    }

    float getMinSize() const
    {
        return m_minSize;
    }

    DynamicOctreeNode* getChild(uint32_t idx)
    {
        IZ_ASSERT(idx < COUNTOF(m_children));
        return m_children[idx];
    }

    uint32_t getRegisteredObjNum() const
    {
        auto num = m_objects.size();
        return num;
    }

    DynamicOctreeObject** getRegisteredObjects()
    {
        return (m_objects.empty() ? nullptr : &m_objects[0]);
    }

private:
    using Result = std::tuple < AddResult, uint32_t > ;

    Result add(
        DynamicOctree* octree,
        DynamicOctreeObject* obj);

    Result addInternal(
        DynamicOctree* octree,
        DynamicOctreeObject* obj);

    // 指定されたオブジェクトを登録可能な子供を探す.
    inline DynamicOctreeNode* findChildCanRegister(DynamicOctreeObject* obj);

    // オブジェクトを無条件で強制登録.
    void addForcibly(
        DynamicOctree* octree,
        DynamicOctreeObject* obj);

    void addChildren(
        DynamicOctree* octree,
        DynamicOctreeNode* children[]);

private:
    AABB m_aabb;

    uint32_t m_depth{ 1 };

    float m_minSize{ 1.0f };

    DynamicOctreeNode* m_children[8];
    std::vector<DynamicOctreeObject*> m_objects;
};

#endif    // #if !defined(__DYNAMIC_OCTREE_NODE_H__)