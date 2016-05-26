#if !defined(__OCTREE_H__)
#define __OCTREE_H__

#include <vector>

#include "izDefs.h"
#include "izMath.h"

#include "Node.h"

// NOTE
// http://marupeke296.com/COL_2D_No8_QuadTree.html
// http://marupeke296.com/COL_3D_No15_Octree.html

struct MortonNumber {
    IZ_UINT number{ 0 };
    IZ_UINT level{ 0 };

    MortonNumber() {}
    MortonNumber(IZ_UINT _n, IZ_UINT _l) : number(_n), level(_l) {}
};

/** Octree.
 */
class Octree {
public:
    static const IZ_UINT MAX_LEVEL = 7;

public:
    Octree() {}
    ~Octree()
    {
        clear();
    }

    NO_COPIABLE(Octree);

private:
    static inline IZ_UINT getBit(IZ_UINT n);

    static inline void getPosFromMortonNumber(
        const MortonNumber& mortonNumber,
        IZ_UINT& outX, IZ_UINT& outY, IZ_UINT& outZ);

public:
    // Compute morton number.
    void getMortonNumberByLevel(
        MortonNumber& ret,
        const IZ_FLOAT* point,
        IZ_UINT level);

    // Initialize octree.
    IZ_BOOL initialize(
        const izanagi::math::SVector4& vMin,
        const izanagi::math::SVector4& vMax,
        IZ_UINT level);

    IZ_UINT getMaxLevel() const
    {
        return m_level;
    }

    // Clear octree.
    void clear();

    // Get node by morton number.
    Node* getNode(const MortonNumber& mortonNumber);

    // Get count of nodes which the octree has.
    IZ_UINT getNodeCount() const
    {
        return m_nodeCount;
    }

    IZ_UINT getNodeCount(IZ_UINT level) const
    {
        IZ_ASSERT(level < m_level);
        return m_nodesNum[level];
    }

    // Get list of nodes which the octree has.
    Node** getNodes()
    {
        return m_nodes;
    }

    Node** getNodes(IZ_UINT level)
    {
        return m_nodesHash[level];
    }

private:
    // Create node.
    Node* createNode(const MortonNumber& mortonNumber);

private:
    static inline IZ_UINT separeteBit(IZ_UINT n)
    {
        IZ_UINT s = n;
        s = (s | s << 8) & 0x0000f00f;
        s = (s | s << 4) & 0x000c30c3;
        s = (s | s << 2) & 0x00249249;
        return s;
    }

private:
    Node** m_nodes{ nullptr };
    Node** m_nodesHash[MAX_LEVEL];

    // ノード総数.
    IZ_UINT m_nodeCount{ 0 };

    // 分割レベル.
    IZ_UINT m_level{ 0 };

    // 各レベルでのノード数.
    IZ_UINT m_nodesNum[MAX_LEVEL];

    union Vector {
        __m128 m;
        izanagi::math::SVector4 v;
    };

    Vector m_min;
    Vector m_max;

    // 全体サイズ.
    Vector m_size;

    // 最小単位サイズ.
    Vector m_units[MAX_LEVEL];
    Vector m_divUnits[MAX_LEVEL];
};

#endif  // #if !defined(__OCTREE_H__)
