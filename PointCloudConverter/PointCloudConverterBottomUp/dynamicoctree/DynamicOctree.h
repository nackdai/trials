#if !defined(__DYNAMIC_OCTREE_H__)
#define __DYNAMIC_OCTREE_H__

#include <stdint.h>
#include <list>
#include "izMath.h"

#define USE_STL_LIST

class DynamicOctreeObject;
class DynamicOctreeNodeBase;

class DynamicOctreeBase {
    friend class DynamicOctreeNodeBase;
    template <typename Obj> friend class DynamicOctreeNode;

protected:
    DynamicOctreeBase() {}
    virtual ~DynamicOctreeBase() {}

public:
    void clear();

    uint32_t getMaxDepth() const
    {
        return m_maxDepth;
    }

    uint32_t getDepth() const
    {
        return m_depth;
    }

    uint64_t getResisteredNum() const
    {
        return m_registeredNum;
    }

    void merge(uint32_t targetDepth);

    virtual void cleanDeleteList() = 0;

protected:
    uint32_t incrementDepth()
    {
        m_depth++;
        return m_depth;
    }
    uint32_t decrementDepth()
    {
        IZ_ASSERT(m_depth > 0);
        m_depth--;
        return m_depth;
    }

    uint32_t setDepthForExpand(uint32_t depth)
    {
        m_depth = (depth > m_depth ? depth : m_depth);
        return m_depth;
    }
    uint32_t setDepthForMerge(uint32_t depth)
    {
        m_depth = (depth < m_depth ? depth : m_depth);
        return m_depth;
    }

    uint32_t getNewIdx(
        int32_t dirX,
        int32_t dirY,
        int32_t dirZ);

    using NodeDir = std::tuple < int32_t, int32_t, int32_t >;

    NodeDir getNodeDir(uint32_t idx);

    virtual void addNode(DynamicOctreeNodeBase* node) = 0;
    virtual void removeNode(DynamicOctreeNodeBase* node, bool willDelete = false) = 0;

protected:
    uint32_t m_maxDepth{ 0 };
    uint32_t m_depth{ 0 };

    uint64_t m_registeredNum{ 0 };
};

/////////////////////////////////////////////////////////

template <typename Node>
class DynamicOctree : public DynamicOctreeBase {
    template <typename OBJ>
    friend class DynamicOctreeNode;

public:
    DynamicOctree() {}
    virtual ~DynamicOctree()
    {
        clear();
    }

public:
    DynamicOctreeNodeBase* getRootNode()
    {
        return m_root;
    }

    void init(
        float initialSize,
        const izanagi::math::SVector4& initialPos,
        float minSize,
        uint32_t maxDepth)
    {
        if (!m_root) {
            m_root = new Node(initialSize, initialPos, minSize);
            m_depth = 1;
            m_maxDepth = maxDepth;

            IZ_ASSERT(m_maxDepth > 0);

            addNode(m_root);
        }
    }

    template <typename OBJ>
    uint32_t add(OBJ obj)
    {
        izanagi::math::CVector3 pos(obj.getCenter().getXYZ());

        uint32_t loopCount = 0;

        uint32_t registeredDepth = 0;

        auto addType = DynamicOctreeNodeBase::AddResult::None;

        for (;;) {
            auto result = m_root->add<Node>(this, obj);

            addType = std::get<0>(result);

            if (addType == DynamicOctreeNodeBase::AddResult::Success) {
                registeredDepth = std::get<1>(result);
                break;
            }
            else if (addType == DynamicOctreeNodeBase::AddResult::NotContain) {
                // 今のルートノードの大きさの範囲外なので広げる.
                auto center = m_root->getCenter();
                expand(pos - center);

                if (m_depth > m_maxDepth) {
                    merge(m_maxDepth);
                }
            }
            else if (addType == DynamicOctreeNodeBase::AddResult::OverFlow) {
#if 0
                // 登録数がオーバーフローしたが、行き先がなくなるので、強制的に登録する.
                m_root->addForcibly(this, obj);
                registeredDepth = 1;
#else
                // 登録数がオーバーフローしたが、誰も受け入れなかったので、リーフノードに強制的に登録する.
                auto leafNode = (Node*)std::get<2>(result);
                IZ_ASSERT(leafNode && !leafNode->hasChildren());

                leafNode->addForcibly(this, obj);
                registeredDepth = leafNode->getDepth();
#endif

                addType = DynamicOctreeNodeBase::AddResult::Success;

                break;
            }
            else {
                // TODO
                IZ_ASSERT(IZ_FALSE);
            }

            if (loopCount > 10) {
                // TODO
                IZ_ASSERT(IZ_FALSE);
                return 0;
            }

            loopCount++;
        }

        if (addType == DynamicOctreeNodeBase::AddResult::Success) {
            m_registeredNum++;
        }

        return registeredDepth;
    }

    void clear()
    {
        if (m_root) {
            delete m_root;
            m_root = nullptr;

            m_depth = 0;
        }
    }

    void merge(uint32_t targetDepth)
    {
        IZ_ASSERT(targetDepth <= m_maxDepth);

        auto result = m_root->merge(
            this,
            targetDepth);

        if (result) {
            m_depth = targetDepth;
        }
    }


    void traverse(std::function<void(Node*)> func)
    {
#ifdef USE_STL_LIST
        auto it = m_nodeList.begin();

        while (it != m_nodeList.end()) {
            auto node = *it;
            func(node);
            it++;
        }
#else
        Node* node = (Node*)m_listTop.m_next;

        while (node != &m_listTail)
        {
            func(node);
            node = node->m_next;
        }
#endif
    }

#ifdef USE_STL_LIST
    std::list<DynamicOctreeNodeBase*>& getNodeList()
    {
        return m_nodeList;
    }
#endif

    virtual void cleanDeleteList() override
    {
        for (uint32_t i = 0; i < m_willDeleteNodeList.size(); i++) {
            auto node = m_willDeleteNodeList[i];

            node->close();

            delete node;
        }
        m_willDeleteNodeList.clear();
    }

private:    
    void expand(const izanagi::math::SVector3& dir)
    {
        int32_t dirX = dir.x >= 0.0f ? 1 : -1;
        int32_t dirY = dir.y >= 0.0f ? 1 : -1;
        int32_t dirZ = dir.z >= 0.0f ? 1 : -1;

        auto prevRoot = m_root;

        auto minPos = m_root->getMin();
        auto center = m_root->getCenter();
        auto size = m_root->getSize();
        auto minSize = m_root->getMinSize();

        izanagi::math::CVector3 half(size);
        half *= 0.5f;

        // 倍に広げる.
        izanagi::math::CVector3 newSize(size);
        newSize *= 2.0f;

        izanagi::math::CVector4 newCenter(center.x, center.y, center.z);
        newCenter.x += dirX * half.x;
        newCenter.y += dirY * half.y;
        newCenter.z += dirZ * half.z;

        // 現在のルートノードを子供とする、新しいルートノードを作る.
        m_root = new Node(
            newSize.x,
            newCenter,
            minSize);

        addNode(m_root);

        incrementDepth();

        auto idx = getNewIdx(dirX, dirY, dirZ);

        DynamicOctreeNodeBase* children[8];

        for (uint32_t i = 0; i < 8; i++) {
            if (idx == i) {
                children[i] = prevRoot;
                prevRoot->incrementDepth();
            }
            else {
                izanagi::math::CVector4 pos(newCenter.x, newCenter.y, newCenter.z);

                auto nodeDir = getNodeDir(i);

                dirX = std::get<0>(nodeDir);
                dirY = std::get<1>(nodeDir);
                dirZ = std::get<2>(nodeDir);

                pos.x += dirX * half.x;
                pos.y += dirY * half.y;
                pos.z += dirZ * half.z;

                auto child = new Node(
                    size.x,
                    pos,
                    minSize);

                addNode(child);

                children[i] = child;
            }
        }

        m_root->addChildren<Node>(this, children);
    }

    virtual void addNode(DynamicOctreeNodeBase* node) override
    {
#ifdef USE_STL_LIST
        m_nodeList.push_back(node);
#else
        DynamicOctreeNode* top = (DynamicOctreeNode*)&m_listTop;
        DynamicOctreeNode* tail = (DynamicOctreeNode*)&m_listTail;

        if (!m_isInitalizedList) {
            m_isInitalizedList = IZ_TRUE;

            top->m_next = tail;
            tail->m_prev = top;
        }

        IZ_ASSERT(node->m_prev == nullptr && node->m_next == nullptr);

        auto prev = tail->m_prev;

        node->m_prev = prev;
        node->m_next = tail;

        prev->m_next = node;
        tail->m_prev = node;
#endif
    }

    virtual void removeNode(DynamicOctreeNodeBase* node, bool willDelete = false) override
    {
        if (willDelete) {
            m_willDeleteNodeList.push_back(node);
        }

#ifdef USE_STL_LIST
        m_nodeList.remove(node);
#else
        if (!m_isInitalizedList) {
            return;
        }

        IZ_ASSERT(node->m_prev && node->m_next);

        auto prev = node->m_prev;
        auto next = node->m_next;

        prev->m_next = next;
        next->m_prev = prev;

        node->m_prev = nullptr;
        node->m_next = nullptr;
#endif
    }

private:
    Node* m_root{ nullptr };

#ifdef USE_STL_LIST
    std::list<DynamicOctreeNodeBase*> m_nodeList;
#else
    IZ_BOOL m_isInitalizedList{ IZ_FALSE };
    Node m_listTop;
    Node m_listTail;
#endif

    std::vector<DynamicOctreeNodeBase*> m_willDeleteNodeList;
};

#endif    // #if !defined(__DYNAMIC_OCTREE_H__)