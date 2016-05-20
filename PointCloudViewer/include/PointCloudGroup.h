#if !defined(__POINT_CLOUD_GROUP_H__)
#define __POINT_CLOUD_GROUP_H__

#include "izSampleKit.h"

class AABB {
public:
    AABB() {}
    ~AABB() {}

public:
    void set(
        const izanagi::math::SVector3& vMin,
        const izanagi::math::SVector3& vMax)
    {
        m_min = vMin;
        m_max = vMax;
    }

    izanagi::math::CVector3 getCenter() const
    {
        izanagi::math::CVector3 center = m_min + m_max;
        center *= 0.5f;

        return std::move(center);
    }

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

class PointDataGroup {
public:
    PointDataGroup() {}
    PointDataGroup(uint32_t limit) : m_maxVtxNum(limit) {}

    ~PointDataGroup()
    {
        release();
    }

public:
    enum Result {
        Succeeded,
        NoExist,
        NotSameFormat,
        Overflow,
        Failed,
    };

    static bool needOtherGroup(Result res)
    {
        return (res == Result::NotSameFormat || res == Result::Overflow);
    }

    static bool isSucceeded(Result res)
    {
        return (res == Result::Succeeded);
    }

    void release();

    Result read(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device,
        const char* path);

    void traverseAABB(std::function<void(const izanagi::math::SMatrix44&)> func);

    izanagi::graph::CVertexDeclaration* getVD()
    {
        return m_vd;
    }

    izanagi::graph::CVertexBuffer* getVB()
    {
        return m_vbDynamicStream;
    }

    uint32_t getVtxSize() const
    {
        return m_vtxSize;
    }

    uint32_t getVtxNum() const
    {
        return m_vtxCnt;
    }

    float getScale() const
    {
        return m_scale;
    }

private:
    std::vector<AABB> m_aabbs;

    izanagi::graph::CVertexDeclaration* m_vd{ nullptr };

    uint32_t m_format{ 0 };

    uint32_t m_maxVtxNum{ 1000 };
    uint32_t m_vtxCnt{ 0 };

    uint32_t m_vtxSize{ 0 };

    float m_scale{ 1.0f };

    GLuint m_glVB{ 0 };
    void* m_mappedDataPtr{ nullptr };
    izanagi::graph::CVertexBuffer* m_vbDynamicStream{ nullptr };
};

#endif    // #if !defined(__POINT_CLOUD_GROUP_H__)