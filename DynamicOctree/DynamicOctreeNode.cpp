#include "DynamicOctreeNode.h"

void AABB::set(
    const izanagi::math::SVector3& size,
    const izanagi::math::SVector4& pos)
{
    m_min.Set(pos.getXYZ());
    m_max.Set(m_min);

    izanagi::math::CVector3 half(size);
    half *= 0.5f;

    m_min -= half;
    m_max += half;
}

bool AABB::isContain(const izanagi::math::SVector4& p)
{
    bool ret = (m_min.x <= p.x && p.x <= m_max.x)
        && (m_min.y <= p.y && p.y <= m_max.y)
        && (m_min.z <= p.z && p.z <= m_max.z);

    return ret;
}

DynamicOctreeNode::DynamicOctreeNode(
    float initialSize,
    const izanagi::math::SVector4& initialPos,
    float minSize)
{
    m_aabb.set(
        izanagi::math::CVector3(initialSize, initialSize, initialSize),
        initialPos);

    m_minSize = minSize;

    m_children[0] = nullptr;
    m_children[1] = nullptr;
    m_children[2] = nullptr;
    m_children[3] = nullptr;
    m_children[4] = nullptr;
    m_children[5] = nullptr;
    m_children[6] = nullptr;
    m_children[7] = nullptr;
}

DynamicOctreeNode::~DynamicOctreeNode()
{
    for (int i = 0; i < COUNTOF(m_children); i++) {
        if (m_children[i]) {
            delete m_children[i];
        }
    }
}

bool DynamicOctreeNode::add(DynamicOctreeObject* obj)
{
    const auto p = obj->getCenter();

    if (!m_aabb.isContain(p)) {
        return false;
    }

    addInternal(obj);

    return true;
}

void DynamicOctreeNode::addInternal(DynamicOctreeObject* obj)
{
    // TODO
}
