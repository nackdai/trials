#if !defined(__DYNAMIC_STREAM_MULTI_DRAW_APP_H__)
#define __DYNAMIC_STREAM_MULTI_DRAW_APP_H__

#include "izSampleKit.h"
#include "bufferlock.h"

#include "VertexStreamManager.h"

//#define ENABLE_TRHEAD

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

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

class VertexInput : public IVertexStreamInput {
public:
    VertexInput() {}
    virtual ~VertexInput() {}

public:
    void init();

    virtual IZ_BOOL needUpdate() override;

    virtual IZ_BOOL canWrite(IZ_UINT size) override;

    virtual IZ_UINT writeToBuffer(void* buffer, IZ_UINT offset) override;

    virtual IZ_UINT getSize() const override;
    virtual IZ_UINT getCount() const override;

    void needUpdateForcibly();

private:
    IZ_BOOL m_needUpdate{ IZ_TRUE };
    Vertex m_vtx[POINT_NUM];
};

class DynamicStreamMultiDrawApp : public izanagi::sample::CSampleApp {
public:
    DynamicStreamMultiDrawApp();
    virtual ~DynamicStreamMultiDrawApp();

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
    void RenderDynamicStream(izanagi::graph::CGraphicsDevice* device);

private:
    VertexStreamManager m_vtxStream;

    VertexInput m_vtxInput[LIST_NUM];

    izanagi::graph::CVertexDeclaration* m_vd{ nullptr };

    izanagi::graph::CVertexShader* m_vs{ nullptr };
    izanagi::graph::CPixelShader* m_ps{ nullptr };

    izanagi::graph::CShaderProgram* m_shd{ nullptr };

    IZ_BOOL m_needUpdate{ IZ_FALSE };
};

#endif    // #if !defined(__DYNAMIC_STREAM_MULTI_DRAW_APP_H__)