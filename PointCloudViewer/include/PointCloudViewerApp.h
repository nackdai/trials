#if !defined(__POINT_CLOUD_VIEWER_APP_H__)
#define __POINT_CLOUD_VIEWER_APP_H__

#include "izSampleKit.h"
#include "izShader.h"

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

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

    ~PointDataGroup() {}

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

private:
    std::vector<AABB> m_aabbs;

    izanagi::graph::CVertexDeclaration* m_vd{ nullptr };

    uint32_t m_format{ 0 };

    uint32_t m_maxVtxNum{ 1000 };
    uint32_t m_vtxCnt{ 0 };

    uint32_t m_vtxSize{ 0 };

    GLuint m_glVB{ 0 };
    void* m_mappedDataPtr{ nullptr };
    izanagi::graph::CVertexBuffer* m_vbDynamicStream{ nullptr };
};

class PointCloudViewerApp : public izanagi::sample::CSampleApp {
public:
    PointCloudViewerApp();
    virtual ~PointCloudViewerApp();

protected:
    // 初期化.
    virtual IZ_BOOL InitInternal(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device,
        izanagi::sample::CSampleCamera& camera);

    // 解放.
    virtual void ReleaseInternal();

    // 更新.
    virtual void UpdateInternal(izanagi::graph::CGraphicsDevice* device);

    // 描画.
    virtual void RenderInternal(izanagi::graph::CGraphicsDevice* device);

    virtual IZ_BOOL OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key) override;

private:
    void searchDir(const char* filename);

private:
    izanagi::graph::CVertexShader* m_vs{ nullptr };
    izanagi::graph::CPixelShader* m_ps{ nullptr };

    izanagi::graph::CShaderProgram* m_shd{ nullptr };

    izanagi::shader::CShaderBasic* m_basicShd{ nullptr };

    izanagi::CDebugMeshWiredBox* m_wiredBox{ nullptr };

    std::vector<std::string> m_files;
    std::vector<PointDataGroup*> m_groups;
};

#endif    // #if !defined(__POINT_CLOUD_VIEWER_APP_H__)