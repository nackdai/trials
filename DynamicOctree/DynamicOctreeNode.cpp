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

uint32_t DynamicOctreeNode::s_maxRegisteredObjCount = 10;

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
    if (m_prev) {
        m_prev->m_next = m_next;
    }
    if (m_next) {
        m_next->m_prev = m_prev;
    }

    for (int i = 0; i < COUNTOF(m_children); i++) {
        if (m_children[i]) {
            delete m_children[i];
        }
    }
}

DynamicOctreeNode::Result DynamicOctreeNode::add(
    DynamicOctreeBase* octree,
    DynamicOctreeObject* obj)
{
    auto p = obj->getCenter();

    if (!m_aabb.isContain(p)) {
        return Result(AddResult::NotContain, m_depth, this);
    }

    auto ret = addInternal(octree, obj);

    return ret;
}

DynamicOctreeNode::Result DynamicOctreeNode::addInternal(
    DynamicOctreeBase* octree,
    DynamicOctreeObject* obj)
{
    Result result = Result(AddResult::None, m_depth, this);

#if 1
    auto maxDepth = octree->getMaxDepth();

    if (m_depth < maxDepth && !m_children[0]) {
        auto size = getSize();
        izanagi::math::CVector3 newSize(size);
        newSize *= 0.5f;

        izanagi::math::CVector3 half(newSize);
        half *= 0.5f;

        auto minSize = getMinSize();

        auto center = getCenter();

        auto parentMortionNumber = m_mortonNumber;

        for (uint32_t i = 0; i < COUNTOF(m_children); i++) {
            izanagi::math::CVector4 pos(center.x, center.y, center.z);

            auto dir = octree->getNodeDir(i);

            auto dirX = std::get<0>(dir);
            auto dirY = std::get<1>(dir);
            auto dirZ = std::get<2>(dir);

            pos.x += dirX * half.x;
            pos.y += dirY * half.y;
            pos.z += dirZ * half.z;

            auto child = new DynamicOctreeNode(
                newSize.x,
                pos,
                minSize);

            child->m_parent = this;
            child->m_depth = m_depth + 1;

            child->setMortonNumber((parentMortionNumber << 3) | i);

            octree->addNode(child);

            m_children[i] = child;
        }

        octree->setDepthForExpand(m_depth + 1);
    }

    if (m_children[0]) {
        auto child = findChildCanRegister(obj);
        result = child->add(octree, obj);
    }
#else
    if (m_children[0]) {
        auto child = findChildCanRegister(obj);
        result = child->add(octree, obj);
    }
#endif

    auto addtype = std::get<0>(result);

    if (addtype == AddResult::OverFlow
        || addtype == AddResult::None)
    {
        auto maxCnt = getMaxRegisteredObjCount();
        auto num = m_objects.size();

        if (num < maxCnt) {
            m_objects.push_back(obj);
            result = Result(AddResult::Success, m_depth, this);
        }
        else {
            auto overFlowedNode = std::get<2>(result);
            result = Result(AddResult::OverFlow, m_depth, overFlowedNode);
        }
    }

    return result;
}

// 指定されたオブジェクトを登録可能な子供を探す.
DynamicOctreeNode* DynamicOctreeNode::findChildCanRegister(DynamicOctreeObject* obj)
{
    const auto center = getCenter();
    const auto pos = obj->getCenter();

    uint32_t idx = 0;
    idx += (pos.x >= center.x ? 1 : 0);
    idx += (pos.y >= center.y ? 2 : 0);
    idx += (pos.z >= center.z ? 4 : 0);

    IZ_ASSERT(m_children[idx]);
    return m_children[idx];
}

// オブジェクトを無条件で強制登録.
void DynamicOctreeNode::addForcibly(
    DynamicOctreeBase* octree,
    DynamicOctreeObject* obj)
{
    auto isContain = m_aabb.isContain(obj->getCenter());
    IZ_ASSERT(isContain);

    if (isContain) {
        m_objects.push_back(obj);
    }
}

void DynamicOctreeNode::addChildren(
    DynamicOctreeBase* octree,
    DynamicOctreeNode* children[])
{
    IZ_ASSERT(m_children[0] == nullptr);

    auto parentMortonNumber = m_mortonNumber;

    for (int i = 0; i < COUNTOF(m_children); i++) {
        m_children[i] = children[i];
        IZ_ASSERT(m_children[i]);

        m_children[i]->m_parent = this;
        m_children[i]->m_depth = this->m_depth + 1;

        m_children[i]->setMortonNumber((parentMortonNumber << 3) | i);
    }

    auto maxCnt = getMaxRegisteredObjCount();
    auto num = m_objects.size();

#if 1
    // 子供に移す.
    for (uint32_t i = 0; i < num; i++) {
        auto obj = m_objects.back();

        auto child = findChildCanRegister(obj);
        auto result = child->add(octree, obj);

        auto addType = std::get<0>(result);

        // 必ず入るはず...
        IZ_ASSERT(addType != AddResult::NotContain);

        if (addType == AddResult::Success) {
            m_objects.pop_back();
        }

        // 子供でも受けきれない場合は、そのまま持っておく.
    }
#else
    // オーバーした分は子供に移す.
    if (num > maxCnt) {
        auto cnt = maxCnt - num;

        for (uint32_t i = 0; i < cnt; i++) {
            auto obj = m_objects.back();

            auto child = findChildCanRegister(obj);
            auto result = child->add(octree, obj);

            auto addType = std::get<0>(result);

            // 必ず入るはず...
            IZ_ASSERT(addType != AddResult::NotContain);

            if (addType == AddResult::Success) {
                m_objects.pop_back();
            }

            // 子供でも受けきれない場合は、そのまま持っておく.
        }
    }
#endif
}

bool DynamicOctreeNode::merge(
    DynamicOctreeBase* octree,
    uint32_t targetDepth)
{
    bool ret = false;

    if (m_depth + 1 > targetDepth) {
        if (m_children[0]) {
            for (int i = 0; i < COUNTOF(m_children); i++) {
                m_children[i]->merge(octree, targetDepth);
                IZ_ASSERT(!m_children[i]->hasChildren());
            }

            auto objNum = 0;

            for (int i = 0; i < COUNTOF(m_children); i++) {
                objNum += m_children[i]->getRegisteredObjNum();
            }

            if (objNum > 0) {
                auto curNum = m_objects.size();

                m_objects.resize(curNum + objNum);

                auto offset = curNum;
                uint8_t* dst = (uint8_t*)this->getRegisteredObjects();

                for (int i = 0; i < COUNTOF(m_children); i++) {
                    auto num = m_children[i]->getRegisteredObjNum();
                    auto src = m_children[i]->getRegisteredObjects();

                    memcpy(dst + offset, src, sizeof(DynamicOctreeObject*) * num);
                    offset += num;
                }
            }

            for (int i = 0; i < COUNTOF(m_children); i++) {
                octree->removeNode(m_children[i]);
                delete m_children[i];
                m_children[i] = nullptr;
            }

            octree->setDepthForMerge(m_depth);

            ret = true;
        }
    }
    else {
        if (m_children[0]) {
            for (int i = 0; i < COUNTOF(m_children); i++) {
                auto result = m_children[i]->merge(octree, targetDepth);

                if (result) {
                    ret = true;
                }
            }
        }
    }

    return ret;
}

void DynamicOctreeNode::incrementDepth()
{
    if (m_children[0]) {
        for (int i = 0; i < COUNTOF(m_children); i++) {
            m_children[i]->incrementDepth();
        }
    }
    m_depth++;
}
