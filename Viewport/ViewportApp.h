#if !defined(__VIEWPORT_APP_H__)
#define __VIEWPORT_APP_H__

#include "izSampleKit.h"
#include "izCollision.h"

static const IZ_UINT SCREEN_WIDTH = 640;
static const IZ_UINT SCREEN_HEIGHT = 360;

class ViewportApp : public izanagi::sample::CSampleApp {
public:
    ViewportApp();
    virtual ~ViewportApp();

protected:
    // 初期化.
    virtual IZ_BOOL InitInternal(
        izanagi::IMemoryAllocator* allocator,
        izanagi::graph::CGraphicsDevice* device,
        izanagi::sample::CSampleCamera& camera) override;

    // 解放.
    virtual void ReleaseInternal() override;

    // 更新.
    virtual void UpdateInternal(izanagi::graph::CGraphicsDevice* device) override;

    // 描画.
    virtual void RenderInternal(izanagi::graph::CGraphicsDevice* device) override;

private:
    static const IZ_UINT CUBE_NUM = 5;

    struct Cube {
        izanagi::CDebugMesh* mesh{ nullptr };
        izanagi::math::SMatrix44 mtxL2W;
        izanagi::col::AABB aabb;
    } m_cubes[CUBE_NUM];

    izanagi::CDebugMeshAxis* m_Axis{ nullptr };
    izanagi::CDebugMeshGrid* m_Grid{ nullptr };

    izanagi::shader::CShaderBasic* m_Shader;

    izanagi::col::Frustum m_frustum[4];
};

#endif    // #if !defined(__VIEWPORT_APP_H__)