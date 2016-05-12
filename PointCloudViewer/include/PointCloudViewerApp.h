#if !defined(__POINT_CLOUD_VIEWER_APP_H__)
#define __POINT_CLOUD_VIEWER_APP_H__

#include "izSampleKit.h"
#include "izShader.h"

#include "PointCloudGroup.h"

#define ENABLE_MANUAL

static const IZ_UINT SCREEN_WIDTH = 1280;
static const IZ_UINT SCREEN_HEIGHT = 720;

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

    izanagi::IMemoryAllocator* m_allocator{ nullptr };
    PointDataGroup* m_curGroup{ nullptr };

    bool m_readData{ false };

    IZ_UINT m_total{ 0 };
};

#endif    // #if !defined(__POINT_CLOUD_VIEWER_APP_H__)