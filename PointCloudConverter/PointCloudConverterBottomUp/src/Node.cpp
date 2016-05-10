#include "Node.h"

std::string Node::BasePath("./");

void Node::flush()
{
    auto registeredNum = getRegisteredObjNum();

    if (registeredNum == 0) {
        return;
    }

    if (!m_fp) {
        std::string path(BasePath);

#if 0
        auto depth = getDepth();
        auto mortonNumber = getMortonNumber();

        path += "r";
        uint32_t mask = 0x07 << (3 * (depth - 1));

        for (uint32_t i = 1; i < depth; i++) {
            auto n = mortonNumber & mask;

            path += n;

            mask >>= 3;
        }

        path += ".spcd";

        fopen_s(&m_fp, path.c_str(), "wb");
        IZ_ASSERT(m_fp);

        // ヘッダー分空ける.
        fseek(m_fp, sizeof(m_header), SEEK_SET);

        m_header.magic_number = FOUR_CC('S', 'P', 'C', 'D');
        m_header.version = 0;

        m_header.depth = depth;
        m_header.mortonNumber = mortonNumber;
#else
        auto id = getId();

        char tmp[10];
        sprintf(tmp, "%d\0", id);

        path += tmp;

        path += ".spcd";

        fopen_s(&m_fp, path.c_str(), "wb");
        IZ_ASSERT(m_fp);

        // ヘッダー分空ける.
        fseek(m_fp, sizeof(m_header), SEEK_SET);

        m_header.magic_number = FOUR_CC('S', 'P', 'C', 'D');
        m_header.version = 0;
#endif
    }

    DynamicOctreeNode<Point>::flush(
        [&](Point* src, uint32_t size) {
        fwrite(src, size, 1, m_fp);
    });

    m_isProcessing = false;
}

void Node::close()
{
    if (m_fp) {
        m_header.vtxNum = getTotalObjNum();

        // TODO
        m_header.vtxFormat = VtxFormat::Position
            | VtxFormat::Color;

        auto depth = getDepth();
        auto mortonNumber = getMortonNumber();
        auto objNum = getTotalObjNum();

        m_header.vtxNum = objNum;

        m_header.depth = depth;
        m_header.mortonNumber = mortonNumber;

        auto fileSize = ftell(m_fp);

        m_header.fileSize = fileSize;
        m_header.fileSizeWithoutHeader = fileSize - sizeof(m_header);

        auto& aabbMin = getMin();
        auto& aabbMax = getMax();

        m_header.aabbMin[0] = aabbMin.x;
        m_header.aabbMin[1] = aabbMin.y;
        m_header.aabbMin[2] = aabbMin.z;

        m_header.aabbMax[0] = aabbMax.x;
        m_header.aabbMax[1] = aabbMax.y;
        m_header.aabbMax[2] = aabbMax.z;

        // 先頭に戻って、ヘッダー書き込み.
        fseek(m_fp, 0, SEEK_SET);
        fwrite(&m_header, sizeof(m_header), 1, m_fp);

        fclose(m_fp);
        m_fp = nullptr;
    }
}
