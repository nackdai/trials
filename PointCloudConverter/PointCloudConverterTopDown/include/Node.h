#if !defined(__NODE_H__)
#define __NODE_H__

#include <atomic>
#include "../SPCDFormat.h"

#include "izCollision.h"
#include "Config.h"

//#define USE_STL_VECTOR

#define ENABLE_HALF_FLOAT

struct Point {
    float pos[3];
    union {
        IZ_COLOR color;
        IZ_UINT8 rgba[4];
    };
};

struct HalfPoint {
    IZ_UINT16 pos[4];
    IZ_UINT8 rgba[4];
};

#ifdef ENABLE_HALF_FLOAT
using ExportPoint = HalfPoint;
#else
using ExportPoint = Point;
#endif

class Node : public izanagi::col::IOctreeNode {
    friend class Writer;

public:
    static std::string BasePath;

    static std::atomic<uint32_t> FlushedNum;

    static float Scale;

private:
    static uint32_t s_ID;

    static std::atomic<uint32_t> CurIdx;

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
            add(vtx);
            return true;
        }
        return false;
    }

    bool add(const Point& vtx);

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
    ExportPoint m_vtx[2][FLUSH_LIMIT];
    uint32_t m_pos[2];
#endif

    uint32_t m_totalNum{ 0 };
};

#endif    // #if !defined(__NODE_H__)