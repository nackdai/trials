#if !defined(__NODE_H__)
#define __NODE_H__

#include <atomic>
#include "../SPCDFormat.h"

#include "izCollision.h"
#include "Config.h"

//#define USE_STL_VECTOR

//#define ENABLE_HALF_FLOAT

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

class Node {
    friend class Writer;
    friend class Octree;

public:
    static std::string BasePath;

    static std::atomic<uint32_t> FlushedNum;

    static float Scale;

private:
    static uint32_t s_ID;

    static std::atomic<uint32_t> CurIdx;

public:
    Node();
    ~Node() {}

public:
    void initialize(
        IZ_UINT mortonNumber,
        IZ_UINT8 level)
    {
        m_mortonNumber = mortonNumber;
        m_level = level;
    }

    izanagi::col::AABB& getAABB()
    {
        return m_aabb;
    }

    bool add(const Point& vtx);

    bool isContain(const Point& vtx)
    {
        return m_aabb.isContain(izanagi::math::CVector3(vtx.pos[0], vtx.pos[1], vtx.pos[2]));
    }

    void flush();
    void close();

    void set(Node* node)
    {
        m_aabb = node->m_aabb;
        m_mortonNumber = node->m_mortonNumber;
        m_level = node->m_level;
    }

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

    IZ_UINT m_mortonNumber{ 0 };
    IZ_UINT8 m_level{ 0 };
};

#endif    // #if !defined(__NODE_H__)