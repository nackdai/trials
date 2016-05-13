#include "Node.h"
#include "izSystem.h"

std::string Node::BasePath("./");

std::atomic<uint32_t> Node::FlushedNum = 0;

uint32_t Node::s_ID = 0;

std::vector<bool> IsOpened(1000);

void Node::flush()
{
    uint32_t idx = m_curIdx;
    m_curIdx = 1 - m_curIdx;

    if (m_vtx[idx].size() == 0) {
        return;
    }

    if (!m_fp) {
        std::string path(BasePath);

#if 0
        auto id = m_id;
#else
        auto id = getNodeArrayIdx();
#endif

        char tmp[10];
        sprintf(tmp, "%d\0", id);

        path += tmp;

        path += ".spcd";

        auto err = fopen_s(&m_fp, path.c_str(), "wb");

        if (!m_fp) {
            IZ_PRINTF("err[%d](%s)(%s)\n", err, path.c_str(), IsOpened[id] ? "open" : "not open");
        }

        IZ_ASSERT(m_fp);
        IsOpened[id] = true;

        // ヘッダー分空ける.
        fseek(m_fp, sizeof(SPCDHeader), SEEK_SET);
    }

    auto& vtx = m_vtx[idx];

    auto src = &vtx[0];
    auto num = vtx.size();
    auto size = num * sizeof(Point);

    m_totalNum += num;

    FlushedNum += num;

    fwrite(src, size, 1, m_fp);

    vtx.clear();
}

void Node::close()
{
    if (m_fp) {
        IZ_ASSERT(m_vtx[0].size() == 0);
        IZ_ASSERT(m_vtx[1].size() == 0);

        SPCDHeader header;

        header.magic_number = FOUR_CC('S', 'P', 'C', 'D');
        header.version = 0;

        // TODO
        header.vtxFormat = VtxFormat::Position
            | VtxFormat::Color;

        auto depth = getLevel();
        auto mortonNumber = getMortonNumber();
        auto objNum = m_totalNum;

        //FlushedNum += objNum;

        header.vtxNum = objNum;

        header.depth = depth;
        header.mortonNumber = mortonNumber;

        auto fileSize = ftell(m_fp);

        header.fileSize = fileSize;
        header.fileSizeWithoutHeader = fileSize - sizeof(header);

        auto& aabbMin = m_aabb.getMin();
        auto& aabbMax = m_aabb.getMax();

        header.aabbMin[0] = aabbMin.x;
        header.aabbMin[1] = aabbMin.y;
        header.aabbMin[2] = aabbMin.z;

        header.aabbMax[0] = aabbMax.x;
        header.aabbMax[1] = aabbMax.y;
        header.aabbMax[2] = aabbMax.z;

        // TODO
        header.spacing = izanagi::math::SVector4::Length2(aabbMin, aabbMax) / 250.0f;

        // 先頭に戻って、ヘッダー書き込み.
        fseek(m_fp, 0, SEEK_SET);
        fwrite(&header, sizeof(header), 1, m_fp);

        fclose(m_fp);
        m_fp = nullptr;
    }
}
