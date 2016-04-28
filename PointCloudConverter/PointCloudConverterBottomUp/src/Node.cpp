#include "Node.h"

std::string Node::BasePath("./");

void Node::flush()
{
    // TODO
    // このあとにmergeされると、depthが変わってしまう.
    // flush が１度でも呼ばれたらmergeされないようにする？.

    if (!m_fp) {
        std::string path(BasePath);

        auto depth = getDepth();
        auto mortonNumber = getMortonNumber();

        path += "r";
        uint32_t mask = 0x07 << (3 * (depth - 1));

        for (uint32_t i = 1; i < depth; i++) {
            auto n = mortonNumber & mask;

            path += n;

            mask >>= 3;
        }

        fopen_s(&m_fp, path.c_str(), "wb");
        IZ_ASSERT(m_fp);

        // ヘッダー分空ける.
        fseek(m_fp, sizeof(m_header), SEEK_SET);

        m_header.magic_number = FOUR_CC('S', 'P', 'C', 'D');
        m_header.version = 0;

        m_header.depth = depth;
        m_header.mortonNumber = mortonNumber;
    }

    DynamicOctreeNode<Point>::flush(
        [&](Point* src, uint32_t size) {
        fwrite(src, size, 1, m_fp);
    });
}

void Node::close()
{
    IZ_ASSERT(m_fp);

    m_header.vtxNum = getTotalObjNum();

    // TODO
    m_header.vtxFormat = VtxFormat::Position
        | VtxFormat::Color;

    // 先頭に戻って、ヘッダー書き込み.
    fseek(m_fp, 0, SEEK_SET);
    fwrite(&m_header, sizeof(m_header), 1, m_fp);

    fclose(m_fp);
    m_fp = nullptr;
}
