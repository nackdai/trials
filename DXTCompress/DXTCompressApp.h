#if !defined(__DXT_COMPRESS_APP_H__)
#define __DXT_COMPRESS_APP_H__

#include "izSampleKit.h"
#include "izThreadModel.h"

#include "DxtEncoder.h"
#include "CaptureManager.h"

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

class DXTCompressApp : public izanagi::sample::CSampleApp {
public:
    DXTCompressApp();
    virtual ~DXTCompressApp();

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
    static const IZ_UINT MAX_MESH_NUM = 1;

    izanagi::CDebugMesh* m_Mesh;

    izanagi::CImage* m_Img;

    izanagi::shader::CShaderBasic* m_Shader;

    struct ObjectInfo {
        izanagi::CDebugMesh* mesh;
        izanagi::math::SMatrix44 mtxL2W;
    } m_objects[MAX_MESH_NUM];

    izanagi::IMemoryAllocator* m_allocator{ nullptr };

    DxtEncoder m_dxtEncoder;
    CaptureManager m_capture;

    GLuint m_fbo{ 0 };
};

#endif    // #if !defined(__DXT_COMPRESS_APP_H__)