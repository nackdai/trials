#if !defined(__HIDDEN_AREA_MESH_APP_H__)
#define __HIDDEN_AREA_MESH_APP_H__

#include "izSampleKit.h"

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

class HiddenAreaMeshApp: public izanagi::sample::CSampleApp {
public:
    HiddenAreaMeshApp();
    virtual ~HiddenAreaMeshApp();

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

    class Shader {
    public:
        Shader() {}
        ~Shader() {}

        bool Init(
            izanagi::graph::CGraphicsDevice* device,
            const char* pathVs, const char* pathPs);

        void Release();

    public:
        izanagi::graph::CVertexShader* m_vs{ nullptr };
        izanagi::graph::CPixelShader* m_fs{ nullptr };
        izanagi::graph::CShaderProgram* m_shd{ nullptr };
    };

    Shader m_shdDrawCube;

    izanagi::math::SMatrix44 m_L2W;

    izanagi::graph::CRenderTarget* m_mask{ nullptr };

    izanagi::graph::CRenderTarget* m_rt{ nullptr };
    izanagi::graph::CRenderTarget* m_depth{ nullptr };

    bool m_canFoveated{ false };

    static const IZ_INT m_Idx = 3;
};

#endif    // #if !defined(__HIDDEN_AREA_MESH_APP_H__)