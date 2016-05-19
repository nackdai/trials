#if !defined(__NODE_H__)
#define __NODE_H__

#include <atomic>
#include "../SPCDFormat.h"

#include "izCollision.h"
#include "Config.h"

//#define USE_STL_VECTOR

struct Point {
    float pos[3];
    //IZ_COLOR color;
    union {
        IZ_COLOR color;
        IZ_UINT8 rgba[4];
    };
};

class Node : public izanagi::col::IOctreeNode {
    friend class Writer;

public:
    static std::string BasePath;

    static std::atomic<uint32_t> FlushedNum;

private:
    static uint32_t s_ID;

public:
    Node();
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

    bool addWithCheck(const Point& vtx)
    {
        if (isContain(vtx))
        {
#ifdef USE_STL_VECTOR
            m_vtx[m_curIdx].push_back(vtx);
#else
            m_vtx[m_curIdx][m_pos[m_curIdx]] = vtx;
            m_pos[m_curIdx]++;
#endif
            return true;
        }
        return false;
    }

    bool add(const Point& vtx)
    {
#ifdef USE_STL_VECTOR
        m_vtx[m_curIdx].push_back(vtx);
#else
        IZ_ASSERT(m_pos[m_curIdx] < FLUSH_LIMIT);
        auto& pos = m_pos[m_curIdx];
        m_vtx[m_curIdx][pos] = vtx;
        ++pos;
#endif
        return true;
    }

    bool isContain(const Point& vtx)
    {
        return m_aabb.isContain(izanagi::math::CVector3(vtx.pos[0], vtx.pos[1], vtx.pos[2]));
    }

    bool canRegister(IZ_UINT maxLevel)
    {
#if 0
        auto level = getLevel();
        auto num = m_totalNum + m_vtx.size();

        if (level == maxLevel) {
            return true;
        }

        auto scale = sqrtf((float)level + 1);

        return (num < 2000 * scale);
#else
        IZ_ASSERT(IZ_FALSE);
        return false;
#endif
    }

    void flush();
    void close();

private:
    uint32_t m_id{ 0 };

    FILE* m_fp{ nullptr };

    izanagi::col::AABB m_aabb;

#ifdef USE_STL_VECTOR
    std::vector<Point> m_vtx[2];
#else
    Point m_vtx[2][FLUSH_LIMIT];
    uint32_t m_pos[2];
#endif

    std::atomic<uint32_t> m_curIdx{ 0 };

    uint32_t m_totalNum{ 0 };
};

#endif    // #if !defined(__NODE_H__)