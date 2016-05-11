#if !defined(__STATIC_OCTREE_APP_H__)
#define __STATIC_OCTREE_APP_H__

//#define ENABLE_MANUAL

#include "izSampleKit.h"
#include "izShader.h"
#include "izCollision.h"

#define ENABLE_MANUAL

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

struct Vertex {
    IZ_FLOAT pos[4];
    IZ_COLOR color;
};

class Node : public izanagi::col::IOctreeNode {
public:
    Node() {}
    virtual ~Node() {}

public:
    virtual void setAABB(const izanagi::col::AABB& aabb) override
    {
        m_aabb = aabb;
    }
    virtual void getAABB(izanagi::col::AABB& aabb) override
    {
        aabb = m_aabb;
    }

    bool add(const Vertex& vtx)
    {
        if (isContain(vtx))
        {
            m_vtx.push_back(vtx);
            return true;
        }
        return false;
    }

    bool isContain(const Vertex& vtx)
    {
        return m_aabb.isContain(izanagi::math::CVector3(vtx.pos[0], vtx.pos[1], vtx.pos[2]));
    }

    bool hasVtx() const
    {
        return (m_vtx.size() > 0);
    }

private:
    izanagi::col::AABB m_aabb;
    std::vector<Vertex> m_vtx;
};

class StaticOctreeApp : public izanagi::sample::CSampleApp {
public:
    StaticOctreeApp();
    virtual ~StaticOctreeApp();

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
    void createVBForDynamicStream(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device,
        IZ_UINT vtxNum);

private:
    static const IZ_UINT POINT_NUM = 10000;

    IZ_UINT m_vtxIdx{ 0 };
    std::vector<Vertex> m_vtx;

    GLuint m_glVB{ 0 };
    void* m_mappedDataPtr{ nullptr };

    GLuint m_bufferSize{ 0 };

    izanagi::graph::CVertexBuffer* m_vbDynamicStream{ nullptr };

    izanagi::graph::CVertexDeclaration* m_vd{ nullptr };

    izanagi::graph::CVertexShader* m_vs{ nullptr };
    izanagi::graph::CPixelShader* m_ps{ nullptr };

    izanagi::graph::CShaderProgram* m_shd{ nullptr };

    IZ_BOOL m_addPoint{ IZ_FALSE };

    izanagi::col::Octree<Node> m_octree;

    izanagi::shader::CShaderBasic* m_basicShd{ nullptr };

    izanagi::CDebugMeshWiredBox* m_wiredBox{ nullptr };
};

#endif    // #if !defined(__STATIC_OCTREE_APP_H__)