#include "Octree.h"
#include "MortonNumberTable.h"

IZ_UINT Octree::getBit(IZ_UINT n)
{
    auto ret = n & 0x01;
    ret |= (n >> 2) & 0x02;
    ret |= (n >> 4) & 0x04;
    ret |= (n >> 6) & 0x08;
    ret |= (n >> 8) & 0x10;
    ret |= (n >> 10) & 0x20;
    ret |= (n >> 12) & 0x40;
    ret |= (n >> 16) & 0x80;
    return ret;
}

void Octree::getPosFromMortonNumber(
    const MortonNumber& mortonNumber,
    IZ_UINT& outX, IZ_UINT& outY, IZ_UINT& outZ)
{
    auto n = mortonNumber.number;

    outX = n & 0x00249249;
    outX = getBit(outX);

    outY = n & 0x00492492;
    outY >>= 1;
    outY = getBit(outY);

    outZ = n & 0x00924924;
    outZ >>= 2;
    outZ = getBit(outZ);
}

/** Initialize octree.
*/
IZ_BOOL Octree::initialize(
    const izanagi::math::SVector4& vMin,
    const izanagi::math::SVector4& vMax,
    IZ_UINT level)
{
    // レベルを決める.
    m_level = IZ_MIN(MAX_LEVEL, level);
    m_level = (m_level == 0 ? MAX_LEVEL : m_level);

    // 各レベルでのノード数とノード総数を計算.
    m_nodeCount = 1;
    m_nodesNum[0] = 1;
    for (IZ_UINT i = 1; i < m_level; i++) {
        m_nodesNum[i] = m_nodesNum[i - 1] * 8;
        m_nodeCount += m_nodesNum[i];
    }

    m_nodes = new Node*[m_nodeCount];
    memset(m_nodes, 0, sizeof(Node*) * m_nodeCount);

    IZ_UINT pos = 0;
    for (IZ_UINT i = 0; i < m_level; i++) {
        m_nodesHash[i] = &m_nodes[pos];
        pos += m_nodesNum[i];
    }

    // 全体サイズを計算.
    izanagi::math::SVector4::Sub(m_size.v, vMax, vMin);
    m_size.v.w = 0.0f;

    // 最小単位サイズを計算.
    IZ_UINT div = (1 << (m_level - 1));
    izanagi::math::SVector4 unit;
    izanagi::math::SVector4::Div(unit, m_size.v, (IZ_FLOAT)div);
    unit.w = 0.0f;

    m_units[m_level - 1].v = unit;

    m_divUnits[m_level - 1].v.x = 1.0f / unit.x;
    m_divUnits[m_level - 1].v.y = 1.0f / unit.y;
    m_divUnits[m_level - 1].v.z = 1.0f / unit.z;
    m_divUnits[m_level - 1].v.w = 0.0f;

    for (IZ_INT i = m_level - 2; i >= 0; i--) {
        m_units[i].v.x = m_units[i + 1].v.x * 2.0f;
        m_units[i].v.y = m_units[i + 1].v.y * 2.0f;
        m_units[i].v.z = m_units[i + 1].v.z * 2.0f;
        m_units[i].v.w = 0.0f;

        m_divUnits[i].v.x = 1.0f / m_units[i].v.x;
        m_divUnits[i].v.y = 1.0f / m_units[i].v.y;
        m_divUnits[i].v.z = 1.0f / m_units[i].v.z;
        m_divUnits[i].v.w = 0.0f;
    }

    m_min.v = vMin;
    m_max.v = vMax;

    m_min.v.w = m_max.v.w = 0.0f;

    return IZ_TRUE;
}

// Clear octree.
void Octree::clear()
{
    for (IZ_UINT i = 0; i < m_nodeCount; i++) {
        if (m_nodes[i]) {
            delete m_nodes[i];
        }
    }

    m_level = 0;
    m_nodeCount = 0;
}

namespace {
    inline uint32_t mortonEncode_LUT(unsigned int x, unsigned int y, unsigned int z)
    {
        // NOTE
        // http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/

        uint32_t answer = 0;

        // we start by shifting the third byte, since we only look at the first 21 bits
        answer = morton256_z[(z >> 16) & 0xFF] |
            morton256_y[(y >> 16) & 0xFF] |
            morton256_x[(x >> 16) & 0xFF];

        // shifting second byte
        answer = answer << 24 | morton256_z[(z >> 8) & 0xFF] |
            morton256_y[(y >> 8) & 0xFF] |
            morton256_x[(x >> 8) & 0xFF];

        // first byte
        answer = answer << 12 |
            morton256_z[(z)& 0xFF] |
            morton256_y[(y)& 0xFF] |
            morton256_x[(x)& 0xFF];

        return answer;
    }
}

// Compute morton number.
void Octree::getMortonNumberByLevel(
    MortonNumber& ret,
    const IZ_FLOAT* point,
    IZ_UINT level)
{
    IZ_ASSERT(level < m_level);

    auto pt = _mm_set_ps(0.0f, point[2], point[1], point[0]);
    auto sub = _mm_sub_ps(pt, m_min.m);

    union {
        __m128 m;
        float v[4];
    } U;

    U.m = _mm_mul_ps(sub, m_divUnits[level].m);

#if 1
    int iX = _mm_cvttss_si32(_mm_load_ss(&U.v[0]));
    int iY = _mm_cvttss_si32(_mm_load_ss(&U.v[1]));
    int iZ = _mm_cvttss_si32(_mm_load_ss(&U.v[2]));
#else
    int iX = _mm_cvtt_ss2si(U.m);
    
    auto m = _mm_shuffle_ps(U.m, U.m, _MM_SHUFFLE(0, 3, 2, 1));
    int iY = _mm_cvtt_ss2si(m);

    m = _mm_shuffle_ps(m, m, _MM_SHUFFLE(0, 3, 2, 1));
    int iZ = _mm_cvtt_ss2si(m);
#endif

    ret.number = mortonEncode_LUT(iX, iY, iZ);
    IZ_ASSERT(ret.number < m_nodesNum[level]);

#if 0
    auto n = separeteBit(iX);
    n |= separeteBit(iY) << 1;
    n |= separeteBit(iZ) << 2;
    IZ_ASSERT(ret.number == n);
#endif

    ret.level = level;
}

// Get node by morton number.
Node* Octree::getNode(const MortonNumber& mortonNumber)
{
    auto idx = mortonNumber.number;
    auto level = mortonNumber.level;

    VRETURN_NULL(idx < m_nodesNum[level]);

    IZ_ASSERT(m_nodes != nullptr);

    Node* ret = m_nodesHash[level][idx];

    if (!ret) {
        // If there is no node, create a new node.
        ret = createNode(mortonNumber);
        m_nodesHash[level][idx] = ret;
    }

    return ret;
}

Node* Octree::getNode(IZ_UINT idx)
{
    Node* ret = m_nodes[idx];

    if (!ret) {
        ret = new Node();
        m_nodes[idx] = ret;

        ret->m_pos[0] = 0;
        ret->m_pos[1] = 0;
    }

    return ret;
}

// Create node.
Node* Octree::createNode(const MortonNumber& mortonNumber)
{
    auto idx = mortonNumber.number;
    auto level = mortonNumber.level;

#if 0
    auto node = m_nodesHash[level][idx];

    if (node) {
        return node;
    }

    node = new Node();
#else
    Node* node = new Node();
#endif

    node->initialize(mortonNumber.number, level);

    // Set AABB.
    {
        IZ_UINT posX, posY, posZ;
        getPosFromMortonNumber(
            mortonNumber,
            posX, posY, posZ);

        izanagi::col::MortonNumber tmp;
        tmp.number = mortonNumber.number;
        tmp.level = mortonNumber.level;

        auto& size = m_units[level];

        // Compute min position.
        izanagi::math::SVector4 minPos;
        minPos.x = m_min.v.x + size.v.x * posX;
        minPos.y = m_min.v.y + size.v.y * posY;
        minPos.z = m_min.v.z + size.v.z * posZ;

        izanagi::math::SVector4 maxPos;
        maxPos.x = minPos.x + size.v.x;
        maxPos.y = minPos.y + size.v.y;
        maxPos.z = minPos.z + size.v.z;

        auto& aabb = node->getAABB();
        aabb.initialize(minPos, maxPos);
    }

    return node;
}