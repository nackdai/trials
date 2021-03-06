#if !defined(__MULTI_DRAW_APP_H__)
#define __MULTI_DRAW_APP_H__

#include "izSampleKit.h"
#include "bufferlock.h"

//#define ENABLE_TRHEAD

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

struct DrawArraysIndirectCommand
{
    GLuint count;
    GLuint primCount;
    GLuint first;
    GLuint baseInstance;
};

class MultiDrawApp : public izanagi::sample::CSampleApp {
public:
    MultiDrawApp();
    virtual ~MultiDrawApp();

protected:
    // 初期化.
    virtual IZ_BOOL InitInternal(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device,
        izanagi::sample::CSampleCamera& camera);

    void createVertices();

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

    void createVBForMapUnmap(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device);

    void RenderDynamicStream(izanagi::graph::CGraphicsDevice* device);
    void RenderMapUnmap(izanagi::graph::CGraphicsDevice* device);

private:
    static const IZ_UINT POINT_NUM = 100;
#ifdef ENABLE_TRHEAD
    static const IZ_UINT LIST_NUM = 10000;
#else
    static const IZ_UINT LIST_NUM = 10000;
#endif

    struct Vertex {
        IZ_FLOAT pos[4];
        IZ_COLOR color;
    };

    IZ_UINT m_vtxIdx{ 0 };
    Vertex m_vtx[LIST_NUM][POINT_NUM];

    GLuint m_glVB{ 0 };
    void* m_mappedDataPtr{ nullptr };

    BufferLockManager m_mgrBufferLock{ true };
    GLuint m_bufferSize{ 0 };

    izanagi::graph::CVertexBuffer* m_vbDynamicStream{ nullptr };
    izanagi::graph::CVertexBuffer* m_vbMapUnmap{ nullptr };

    izanagi::graph::CVertexDeclaration* m_vd{ nullptr };

    izanagi::graph::CVertexShader* m_vs{ nullptr };
    izanagi::graph::CPixelShader* m_ps{ nullptr };

    izanagi::graph::CShaderProgram* m_shd{ nullptr };

#ifdef ENABLE_TRHEAD
    struct ListItem {
        std::atomic<bool> isRenderable{ false };
        IZ_UINT offset;
    } m_items[LIST_NUM];

    std::thread m_thread;
    std::atomic<bool> m_isRunThread{ true };
#endif
};

#endif    // #if !defined(__MULTI_DRAW_APP_H__)