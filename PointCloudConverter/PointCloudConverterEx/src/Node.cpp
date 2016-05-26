#include "Node.h"
#include "izSystem.h"

std::string Node::BasePath("./");

std::atomic<uint32_t> Node::FlushedNum = 0;

uint32_t Node::s_ID = 0;

float Node::Scale = 1.0f;

std::atomic<uint32_t> Node::CurIdx = 0;

std::vector<bool> IsOpened(1000);

Node::Node()
{
    m_id = s_ID;
    s_ID++;

#ifndef USE_STL_VECTOR
    m_pos[0] = m_pos[1] = 0;
#endif
}

bool Node::add(const Point& vtx)
{
#ifdef USE_STL_VECTOR
    m_vtx[m_curIdx].push_back(vtx);
#else
    IZ_ASSERT(m_pos[Node::CurIdx] < FLUSH_LIMIT);
    
    auto& pos = m_pos[Node::CurIdx];

#ifdef ENABLE_HALF_FLOAT
    auto& pt = m_vtx[Node::CurIdx][pos];

    uint32_t* pf = (uint32_t*)vtx.pos;

    // NOTE
    // float -> half
    // ftp://www.fox-toolkit.org/pub/fasthalffloatconversion.pdf

#if 0
    auto i = _mm_setr_epi32(pf[0], pf[1], pf[2], 0);
    auto f0 = _mm_srli_epi32(i, 16);
    auto f2 = _mm_srli_epi32(i, 13);

    i = _mm_setr_epi32(
        (pf[0] & 0x7f800000) - 0x38000000,
        (pf[1] & 0x7f800000) - 0x38000000,
        (pf[2] & 0x7f800000) - 0x38000000,
        0);
    auto f1 = _mm_srli_epi32(i, 13);

    pt.pos[0] = (f0.m128i_u32[0] & 0x8000) | (f1.m128i_u32[0] & 0x7c00) | (f2.m128i_u32[0] & 0x03ff);
    pt.pos[1] = (f0.m128i_u32[1] & 0x8000) | (f1.m128i_u32[1] & 0x7c00) | (f2.m128i_u32[1] & 0x03ff);
    pt.pos[2] = (f0.m128i_u32[2] & 0x8000) | (f1.m128i_u32[2] & 0x7c00) | (f2.m128i_u32[2] & 0x03ff);
#else
    uint32_t f = pf[0];
    pt.pos[0] = ((f >> 16) & 0x8000) | ((((f & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | ((f >> 13) & 0x03ff);

    f = pf[1];
    pt.pos[1] = ((f >> 16) & 0x8000) | ((((f & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | ((f >> 13) & 0x03ff);

    f = pf[2];
    pt.pos[2] = ((f >> 16) & 0x8000) | ((((f & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | ((f >> 13) & 0x03ff);
#endif

    static const uint16_t HalfFloatOne = 15360;
    pt.pos[3] = HalfFloatOne;

    pt.rgba[0] = vtx.rgba[0];
    pt.rgba[1] = vtx.rgba[1];
    pt.rgba[2] = vtx.rgba[2];
    pt.rgba[3] = 0xff;
#else
    //m_vtx[Node::CurIdx][pos] = vtx;
    memcpy(&m_vtx[Node::CurIdx][pos], &vtx, sizeof(vtx));
#endif

    ++pos;
#endif
    return true;
}

void Node::flush()
{
    uint32_t idx = 1 - Node::CurIdx;
#ifndef USE_STL_VECTOR
    uint32_t num = m_pos[idx];
    m_pos[idx] = 0;
#endif

#ifdef USE_STL_VECTOR
    if (m_vtx[idx].size() == 0) {
#else
    if (num == 0) {
#endif
        return;
    }

    if (!m_fp) {
        std::string path(BasePath);

        auto id = m_id;

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
#ifdef USE_STL_VECTOR
    auto num = vtx.size();
#endif
    auto size = num * sizeof(ExportPoint);

    m_totalNum += num;

    FlushedNum += num;

    fwrite(src, size, 1, m_fp);

#ifdef USE_STL_VECTOR
    vtx.clear();
#endif
}

void Node::close()
{
    if (m_fp) {
#ifdef USE_STL_VECTOR
        IZ_ASSERT(m_vtx[0].size() == 0);
        IZ_ASSERT(m_vtx[1].size() == 0);
#else
        IZ_ASSERT(m_pos[0] == 0);
        IZ_ASSERT(m_pos[1] == 0);
#endif

        SPCDHeader header;

        header.magic_number = FOUR_CC('S', 'P', 'C', 'D');
        header.version = 0;

        // TODO
        header.vtxFormat = VtxFormat::Position
            | VtxFormat::Color;

        auto depth = m_level;
        auto mortonNumber = m_mortonNumber;
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

        header.aabbMin[0] = aabbMin.x * Node::Scale;
        header.aabbMin[1] = aabbMin.y * Node::Scale;
        header.aabbMin[2] = aabbMin.z * Node::Scale;

        header.aabbMax[0] = aabbMax.x * Node::Scale;
        header.aabbMax[1] = aabbMax.y * Node::Scale;
        header.aabbMax[2] = aabbMax.z * Node::Scale;

        // TODO
        header.spacing = izanagi::math::SVector4::Length2(aabbMin, aabbMax) / 250.0f;

        header.scale = Node::Scale;

        // 先頭に戻って、ヘッダー書き込み.
        fseek(m_fp, 0, SEEK_SET);
        fwrite(&header, sizeof(header), 1, m_fp);

        fclose(m_fp);
        m_fp = nullptr;
    }
}
