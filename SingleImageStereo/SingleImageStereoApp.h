#if !defined(__SINGLE_IMAGE_STEREO_APP_H__)
#define __SINGLE_IMAGE_STEREO_APP_H__

#include "izSampleKit.h"

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

class SingleImageStereoApp: public izanagi::sample::CSampleApp {
public:
    SingleImageStereoApp();
    virtual ~SingleImageStereoApp();

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

    virtual IZ_BOOL OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key);

private:
    izanagi::sample::CSampleEnvBox* m_Cube;

    izanagi::CImage* m_Img;
    
    izanagi::graph::CVertexShader* m_vs{ nullptr };
    izanagi::graph::CPixelShader* m_fs{ nullptr };
    izanagi::graph::CShaderProgram* m_shd{ nullptr };

    izanagi::math::SMatrix44 m_L2W;

    izanagi::graph::CRenderTarget* m_rt{ nullptr };
    izanagi::graph::CRenderTarget* m_depth{ nullptr };

    // 瞳孔間距離.
    static const IZ_FLOAT IPD;

    IZ_FLOAT m_StereoScreenWidth{ 0.0f };

    static const IZ_INT m_Idx = 1;
};

#endif    // #if !defined(__SINGLE_IMAGE_STEREO_APP_H__)