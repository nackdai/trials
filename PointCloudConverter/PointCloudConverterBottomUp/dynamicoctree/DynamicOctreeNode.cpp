#include "DynamicOctreeNode.h"
#include "DynamicOctree.h"

void AABB::set(
    const izanagi::math::SVector3& size,
    const izanagi::math::SVector4& pos)
{
    m_min.Set(pos.x, pos.y, pos.z);
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

izanagi::math::CVector3 AABB::getCenter() const
{
    izanagi::math::CVector3 center = m_min + m_max;
    center *= 0.5f;

    return std::move(center);
}

uint32_t DynamicOctreeNodeBase::s_maxRegisteredObjCount = 10;
