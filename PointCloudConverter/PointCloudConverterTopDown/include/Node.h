#if !defined(__NODE_H__)
#define __NODE_H__

#include <atomic>
#include "../SPCDFormat.h"

#include "izCollision.h"

struct Point {
    float pos[3];
    IZ_COLOR color;
};

class Node : public izanagi::col::IOctreeNode {
public:
    static std::string BasePath;

    static std::atomic<uint32_t> FlushedNum;

private:
    static uint32_t s_ID;

public:
    Node()
    {
        m_id = s_ID;
        s_ID++;
    }
    virtual ~Node() {}

public:
    virtual void setAABB(const izanagi::col::AABB& aabb) override
    {
        m_aabb = aabb;
    }
    virtual void getAABB(izanagi::col::AABB& aabb) override
    {
        aabb = m_aabb;
    }

    bool add(const Point& vtx)
    {
        if (isContain(vtx))
        {
            m_vtx.push_back(vtx);
            return true;
        }
        return false;
    }

    bool isContain(const Point& vtx)
    {
        return m_aabb.isContain(izanagi::math::CVector3(vtx.pos[0], vtx.pos[1], vtx.pos[2]));
    }

    bool hasVtx() const
    {
        return (m_vtx.size() > 0);
    }

    bool canRegister(IZ_UINT maxLevel)
    {
        auto level = getLevel();
        auto num = m_totalNum + m_vtx.size();

        if (level == maxLevel) {
            return true;
        }

        auto scale = sqrtf((float)level + 1);

        return (num < 2000 * scale);
    }

    void flush();
    void close();

private:
    uint32_t m_id{ 0 };

    FILE* m_fp{ nullptr };

    izanagi::col::AABB m_aabb;
    std::vector<Point> m_vtx;

    uint32_t m_totalNum{ 0 };
};

#endif    // #if !defined(__NODE_H__)