#include "DynamicOctreeRenderer.h"

DynamicOctreeRenderer DynamicOctreeRenderer::s_instance;

bool DynamicOctreeRenderer::init(
    izanagi::graph::CGraphicsDevice* device,
    izanagi::IMemoryAllocator* allocator)
{
    terminate();

    m_wiredBox = izanagi::CDebugMeshWiredBox::create(
        allocator,
        device,
        IZ_COLOR_RGBA(0xff, 0xff, 0x00, 0xff),
        1.0f, 1.0f, 1.0f);
    IZ_ASSERT(m_wiredBox);

    return (m_wiredBox != nullptr);
}

void DynamicOctreeRenderer::terminate()
{
    SAFE_RELEASE(m_wiredBox);
    m_wiredBox = nullptr;
}

void DynamicOctreeRenderer::renderNodeRecursivly(
    izanagi::graph::CGraphicsDevice* device,
    DynamicOctreeNode* node,
    DynamicOctreeRenderer::PreRenderer prerenderer)
{
    renderNode(device, node, prerenderer);

    for (uint32_t i = 0; i < 8; i++) {
        auto child = node->getChild(i);
        if (child) {
            renderNodeRecursivly(device, child, prerenderer);
        }
        else {
            break;
        }
    }
}

void DynamicOctreeRenderer::renderNode(
    izanagi::graph::CGraphicsDevice* device,
    DynamicOctreeNode* node,
    DynamicOctreeRenderer::PreRenderer prerenderer)
{
    if (prerenderer) {
        const auto center = node->getCenter();
        auto size = node->getSize();

        izanagi::math::CMatrix44 mtxL2W;
        mtxL2W.SetScale(size.x, size.y, size.z);
        mtxL2W.Trans(center.x, center.y, center.z);

        prerenderer(mtxL2W);
    }

    m_wiredBox->Draw(device);
}
