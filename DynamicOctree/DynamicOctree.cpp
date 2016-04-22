#include "DynamicOctree.h"
#include "DynamicOctreeNode.h"

DynamicOctree::DynamicOctree()
{
}

DynamicOctree::~DynamicOctree()
{
}

void DynamicOctree::init(
    float initialSize,
    const izanagi::math::SVector4& initialPos,
    float minSize)
{
    if (!m_root) {
        m_root = new DynamicOctreeNode(initialSize, initialPos, minSize);
    }
}

void DynamicOctree::add(DynamicOctreeObject* obj)
{
    izanagi::math::CVector3 pos(obj->getCenter().getXYZ());

    uint32_t loopCount = 0;

    while (!m_root->add(obj)) {
        auto center = m_root->getCenter();
        expand(pos - center);

        if (loopCount > 20) {
            // TODO
            IZ_ASSERT(IZ_FALSE);
            return;
        }

        loopCount++;
    }
}

void DynamicOctree::expand(const izanagi::math::SVector3& dir)
{
    int32_t dirX = dir.x >= 0.0f ? 1 : -1;
    int32_t dirY = dir.y >= 0.0f ? 1 : -1;
    int32_t dirZ = dir.z >= 0.0f ? 1 : -1;

    auto prevRoot = m_root;

    const auto& center = m_root->getCenter();
    auto size = m_root->getSize();

    izanagi::math::CVector3 half(size);
    half *= 0.5f;

    izanagi::math::CVector3 newSize(size);
    newSize *= 2.0f;

    izanagi::math::CVector4 newCenter(center.x, center.y, center.z);
    newCenter.x += dirX * half.x;
    newCenter.y += dirY * half.y;
    newCenter.z += dirZ * half.z;

    // 現在のルートノードを子供とする、新しいルートノードを作る.
    m_root = new DynamicOctreeNode(
        newSize.x,
        newCenter,
        prevRoot->getMinSize());

    auto idx = getNewIdx(dirX, dirY, dirZ);

    for (uint32_t i = 0; i < 8; i++) {
        if (idx == i) {
            m_root->addChild(i, prevRoot);
        }
        else {
            auto child = new DynamicOctreeNode();
            m_root->addChild(i, child);
        }
    }
}

int32_t DynamicOctree::getNewIdx(
    float dirX,
    float dirY,
    float dirZ)
{
    int32_t ret = (dirX > 0.0f ? 0 : 1);
    ret += (dirY < 0.0f ? 4 : 0);
    ret += (dirZ < 0.0f ? 2 : 0);
}