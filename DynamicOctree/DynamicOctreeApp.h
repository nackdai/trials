#if !defined(__DYNAMIC_OCTREE_APP_H__)
#define __DYNAMIC_OCTREE_APP_H__

#include "izSampleKit.h"
#include "izShader.h"
#include "DynamicOctree.h"
#include "DynamicOctreeNode.h"

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

struct Vertex {
    IZ_FLOAT pos[4];
    IZ_COLOR color;
};

class DynamicOctreeApp : public izanagi::sample::CSampleApp {
public:
    DynamicOctreeApp();
    virtual ~DynamicOctreeApp();

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
        izanagi::graph::CGraphicsDevice* device);

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
    IZ_BOOL m_willMerge{ IZ_FALSE };

    DynamicOctree<DynamicOctreeNode> m_octree;
    izanagi::shader::CShaderBasic* m_basicShd{ nullptr };
};

#endif    // #if !defined(__DYNAMIC_OCTREE_APP_H__)