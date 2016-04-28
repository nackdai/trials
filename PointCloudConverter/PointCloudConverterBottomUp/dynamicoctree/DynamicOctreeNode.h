#if !defined(__DYNAMIC_OCTREE_NODE_H__)
#define __DYNAMIC_OCTREE_NODE_H__

#include <stdint.h>
#include "izMath.h"

class DynamicOctreeBase;

//////////////////////////////////////////////////////

class AABB {
public:
    AABB() {}
    ~AABB() {}

public:
    void set(
        const izanagi::math::SVector3& size,
        const izanagi::math::SVector4& pos);

    bool isContain(const izanagi::math::SVector4& p);

    izanagi::math::CVector3 getCenter() const;

    izanagi::math::CVector3 getSize() const
    {
        izanagi::math::CVector3 size(m_max);
        size -= m_min;
        return std::move(size);
    }

    const izanagi::math::CVector3& getMin() const
    {
        return m_min;
    }

    const izanagi::math::CVector3& getMax() const
    {
        return m_max;
    }

private:
    izanagi::math::CVector3 m_min;
    izanagi::math::CVector3 m_max;
};

//////////////////////////////////////////////////////

class DynamicOctreeNodeBase {
    friend class DynamicOctreeBase;
    template<typename Node> friend class DynamicOctree;

protected:
    static uint32_t s_maxRegisteredObjCount;

    enum AddResult {
        None,
        Success,
        NotContain,
        OverFlow,
    };

    using Result = std::tuple < AddResult, uint32_t, DynamicOctreeNodeBase* >;

public:
    static void setMaxRegisteredObjCount(uint32_t cnt)
    {
        s_maxRegisteredObjCount = cnt;
    }
    static uint32_t getMaxRegisteredObjCount()
    {
        return s_maxRegisteredObjCount;
    }

public:
    izanagi::math::CVector3 getCenter() const
    {
        auto center = m_aabb.getCenter();
        return std::move(center);
    }

    const izanagi::math::CVector3& getMin() const
    {
        return m_aabb.getMin();
    }

    izanagi::math::CVector3 getSize() const
    {
        return m_aabb.getSize();
    }

    float getMinSize() const
    {
        return m_minSize;
    }

    uint32_t getDepth() const
    {
        return m_depth;
    }

    uint32_t getMortonNumber() const
    {
        return m_mortonNumber;
    }

    virtual DynamicOctreeNodeBase* getChild(uint32_t idx) = 0;

private:
    virtual bool hasChildren() const = 0;

protected:
    AABB m_aabb;

    uint32_t m_depth{ 1 };
    uint32_t m_mortonNumber{ 0 };

    float m_minSize{ 1.0f };

    DynamicOctreeNodeBase* m_next{ nullptr };
    DynamicOctreeNodeBase* m_prev{ nullptr };
};

//////////////////////////////////////////////////////

template <typename Obj>
class DynamicOctreeNode : public DynamicOctreeNodeBase {
    template <typename Node> friend class DynamicOctree;

protected:
    DynamicOctreeNode() {}

    DynamicOctreeNode(
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

    ~DynamicOctreeNode()
    {
#if 0
        if (m_prev) {
            m_prev->m_next = m_next;
        }
        if (m_next) {
            m_next->m_prev = m_prev;
        }
#endif

        for (int i = 0; i < COUNTOF(m_children); i++) {
            if (m_children[i]) {
                delete m_children[i];
            }
        }
    }

public:
    uint32_t getRegisteredObjNum() const
    {
        return m_objects.size();
    }

    Obj* getRegisteredObjects()
    {
        if (m_objects.empty()) {
            return nullptr;
        }

        return &m_objects[0];
    }

protected:
    void flush(std::function<void(Obj*, uint32_t)> func)
    {
        Obj* src = getRegisteredObjects();
        uint32_t num = getRegisteredObjNum();

        if (num > 0) {
            func(src, sizeof(Obj) * num);

            m_totalObjNum += num;
            m_objects.clear();
        }
    }

    uint32_t getTotalObjNum() const
    {
        return m_totalObjNum;
    }

private:
    Result add(
        DynamicOctreeBase* octree,
        Obj obj)
    {
        auto p = obj.getCenter();

        if (!m_aabb.isContain(p)) {
            return Result(AddResult::NotContain, m_depth, this);
        }

        auto ret = addInternal(octree, obj);

        return ret;
    }

    Result addInternal(
        DynamicOctreeBase* octree,
        Obj obj)
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

                auto child = new DynamicOctreeNode<Obj>(
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
    DynamicOctreeNode* findChildCanRegister(Obj obj)
    {
        const auto center = getCenter();
        const auto pos = obj.getCenter();

        uint32_t idx = 0;
        idx += (pos.x >= center.x ? 1 : 0);
        idx += (pos.y >= center.y ? 2 : 0);
        idx += (pos.z >= center.z ? 4 : 0);

        IZ_ASSERT(m_children[idx]);
        return m_children[idx];
    }

    // オブジェクトを無条件で強制登録.
    void addForcibly(
        DynamicOctreeBase* octree,
        Obj obj)
    {
        auto isContain = m_aabb.isContain(obj.getCenter());
        IZ_ASSERT(isContain);

        if (isContain) {
            m_objects.push_back(obj);
        }
    }

    void addChildren(
        DynamicOctreeBase* octree,
        DynamicOctreeNodeBase* children[])
    {
        IZ_ASSERT(m_children[0] == nullptr);

        auto parentMortonNumber = m_mortonNumber;

        for (int i = 0; i < COUNTOF(m_children); i++) {
            m_children[i] = (DynamicOctreeNode*)children[i];
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

    bool merge(
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

                        memcpy(dst + offset, src, sizeof(Obj) * num);
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

    void incrementDepth()
    {
        if (m_children[0]) {
            for (int i = 0; i < COUNTOF(m_children); i++) {
                m_children[i]->incrementDepth();
            }
        }
        m_depth++;
    }

    virtual DynamicOctreeNodeBase* getChild(uint32_t idx) override
    {
        IZ_ASSERT(idx < COUNTOF(m_children));
        return m_children[idx];
    }

    virtual bool hasChildren() const override
    {
        return (m_children[0] != nullptr);
    }

    void setMortonNumber(uint32_t n)
    {
        m_mortonNumber = n;
    }

private:
    std::vector<Obj> m_objects;

    uint32_t m_totalObjNum{ 0 };

    DynamicOctreeNode* m_parent{ nullptr };
    DynamicOctreeNode* m_children[8];
};

#endif    // #if !defined(__DYNAMIC_OCTREE_NODE_H__)