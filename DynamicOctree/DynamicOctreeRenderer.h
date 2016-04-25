#if !defined(__DYNAMIC_OCTREE_RENDERER_H__)
#define __DYNAMIC_OCTREE_RENDERER_H__

#include "DynamicOctreeNode.h"
#include "izDebugUtil.h"

class DynamicOctreeRenderer {
    static DynamicOctreeRenderer s_instance;

public:
    static DynamicOctreeRenderer& instance()
    {
        return s_instance;
    }

private:
    DynamicOctreeRenderer() {}
    ~DynamicOctreeRenderer()
    {
        terminate();
    }

public:
    bool init(
        izanagi::graph::CGraphicsDevice* device,
        izanagi::IMemoryAllocator* allocator);

    void terminate();

    using PreRenderer = std::function < void(const izanagi::math::SMatrix44&) > ;

    void renderNodeRecursivly(
        izanagi::graph::CGraphicsDevice* device,
        DynamicOctreeNode* node,
        PreRenderer prerenderer);

    void renderNode(
        izanagi::graph::CGraphicsDevice* device,
        DynamicOctreeNode* node,
        PreRenderer prerenderer);

private:
    izanagi::CDebugMeshWiredBox* m_wiredBox{ nullptr };
};

#endif    // #if !defined(__DYNAMIC_OCTREE_RENDERER_H__)