#include "DynamicOctree.h"
#include "DynamicOctreeNode.h"

uint32_t DynamicOctreeBase::getNewIdx(
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

DynamicOctreeBase::NodeDir DynamicOctreeBase::getNodeDir(uint32_t idx)
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

    // 中心からの方向を計算.

    auto dirX = ((idx & 0x01) == 0 ? -1 : 1);  // 左（偶数）or 右（奇数）.
    auto dirZ = (idx >= 4 ? 1 : -1);           // 奥（4-7） or 手前（0-3).

    // NOTE
    // 上は 2, 3, 6, 7.
    // ２進数にすると、001, 011, 110, 111 で ２ビット目が立っている.
    // 下は 0, 1, 4, 5.
    // ２進数にすると、000, 001, 100, 101 で ２ビット目が立っていない.
    auto dirY = ((idx & 0x02) > 0 ? 1 : -1);   // 上（２ビット目が立っている）or 下（２ビット目が立っていない）.

    return NodeDir(dirX, dirY, dirZ);
}
