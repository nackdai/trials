#include "DynamicOctree.h"
#include "DynamicOctreeNode.h"

DynamicOctree::DynamicOctree()
{
}

DynamicOctree::~DynamicOctree()
{
    clear();
}

void DynamicOctree::init(
    float initialSize,
    const izanagi::math::SVector4& initialPos,
    float minSize,
    uint32_t maxDepth)
{
    if (!m_root) {
        m_root = new DynamicOctreeNode(initialSize, initialPos, minSize);
        m_depth = 1;
        m_maxDepth = maxDepth;

        IZ_ASSERT(m_maxDepth > 0);
    }
}

void DynamicOctree::clear()
{
    if (m_root) {
        delete m_root;
        m_root = nullptr;

        m_depth = 0;
    }
}

uint32_t DynamicOctree::add(DynamicOctreeObject* obj)
{
    izanagi::math::CVector3 pos(obj->getCenter().getXYZ());

    uint32_t loopCount = 0;

    uint32_t registeredDepth = 0;

    for (;;) {
        auto result = m_root->add(this, obj);

        auto addType = std::get<0>(result);

        if (addType == DynamicOctreeNode::AddResult::Success) {
            registeredDepth = std::get<1>(result);
            break;
        }
        else if (addType == DynamicOctreeNode::AddResult::NotContain) {
            // 今のルートノードの大きさの範囲外なので広げる.
            auto center = m_root->getCenter();
            expand(pos - center);
        }
        else if (addType == DynamicOctreeNode::AddResult::OverFlow) {
            // 登録数がオーバーフローしたが、行き先がなくなるので、強制的に登録する.
            m_root->addForcibly(this, obj);
            registeredDepth = 1;
            break;
        }
        else {
            // TODO
            IZ_ASSERT(IZ_FALSE);
        }

        if (loopCount > 4) {
            // TODO
            //IZ_ASSERT(IZ_FALSE);
            return 0;
        }

        loopCount++;
    }

    return registeredDepth;
}

void DynamicOctree::expand(const izanagi::math::SVector3& dir)
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
    m_root = new DynamicOctreeNode(
        newSize.x,
        newCenter,
        minSize);

    m_depth++;

    auto idx = getNewIdx(dirX, dirY, dirZ);

    DynamicOctreeNode* children[8];

    for (uint32_t i = 0; i < 8; i++) {
        if (idx == i) {
            children[i] = prevRoot;
        }
        else {
            izanagi::math::CVector4 pos(newCenter.x, newCenter.y, newCenter.z);

            // NOTE
            /*
            * y
            * | +----+----+
            * | | 2  | 3  |
            * | +----+----+
            * | | 0  | 1  |
            * | +----+----+
            * +------------->x
            */
            /*
            * y
            * |     +----+----+
            * |  z  | 6  | 7  |
            * |  /  +----+----+
            * | /   | 4  | 5  |
            * |/    +----+----+
            * +------------->x
            */

            // 中心からの方向を計算.

            dirX = ((i & 0x01) == 0 ? -1 : 1);  // 左（偶数）or 右（奇数）.
            dirZ = (i >= 4 ? 1 : -1);           // 奥（4-7） or 手前（0-3).

            // NOTE
            // 上は 2, 3, 6, 7.
            // ２進数にすると、001, 011, 110, 111 で ２ビット目が立っている.
            // 下は 0, 1, 4, 5.
            // ２進数にすると、000, 001, 100, 101 で ２ビット目が立っていない.
            dirY = ((i & 0x02) > 0 ? 1 : -1);   // 上（２ビット目が立っている）or 下（２ビット目が立っていない）.

            pos.x += dirX * half.x;
            pos.y += dirY * half.y;
            pos.z += dirZ * half.z;

            auto child = new DynamicOctreeNode(
                size.x,
                pos,
                minSize);

            children[i] = child;
        }
    }

    m_root->addChildren(this, children);
}

uint32_t DynamicOctree::getNewIdx(
    int32_t dirX,
    int32_t dirY,
    int32_t dirZ)
{
    // NOTE
    /*
    * y
    * | +----+----+
    * | | 2  | 3  |
    * | +----+----+
    * | | 0  | 1  | 
    * | +----+----+
    * +------------->x
    */
    /*
    * y
    * |     +----+----+
    * |  z  | 6  | 7  |
    * |  /  +----+----+
    * | /   | 4  | 5  |
    * |/    +----+----+
    * +------------->x
    */

    uint32_t ret = (dirX < 0 ? 1 : 0);
    ret += (dirY < 0 ? 2 : 0);
    ret += (dirZ < 0 ? 4 : 0);

    return ret;
}