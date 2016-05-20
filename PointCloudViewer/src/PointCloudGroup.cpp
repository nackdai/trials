#include "PointCloudGroup.h"
#include "SPCDReader.h"

void PointDataGroup::release()
{
    m_vbDynamicStream->overrideNativeResource(nullptr);
    CALL_GL_API(::glDeleteBuffers(1, &m_glVB));

    SAFE_RELEASE(m_vbDynamicStream);

    SAFE_RELEASE(m_vd);
}

namespace {
    uint32_t computeVtxSize(uint32_t fmt)
    {
        uint32_t size = 0;

        if ((fmt & VtxFormat::Position) > 0) {
            //size += sizeof(float) * 3;
            size += sizeof(IZ_UINT16) * 4;
        }
        if ((fmt & VtxFormat::Color) > 0) {
            size += sizeof(uint8_t) * 4;
        }
        if ((fmt & VtxFormat::Normal) > 0) {
            size += sizeof(float) * 3;
        }

        return size;
    }

    uint32_t makeVtxElement(
        uint32_t fmt,
        izanagi::graph::SVertexElement* elem)
    {
        uint32_t pos = 0;
        uint32_t offset = 0;

        if ((fmt & VtxFormat::Position) > 0) {
            elem[pos].Stream = 0;
            elem[pos].Offset = offset;
            //elem[pos].Type = izanagi::graph::E_GRAPH_VTX_DECL_TYPE_FLOAT3;
            elem[pos].Type = izanagi::graph::E_GRAPH_VTX_DECL_TYPE_FLOAT16_4;
            elem[pos].Usage = izanagi::graph::E_GRAPH_VTX_DECL_USAGE_POSITION;
            elem[pos].UsageIndex = 0;

            pos++;
            //offset += sizeof(float) * 3;
            offset += sizeof(IZ_UINT16) * 4;
        }
        if ((fmt & VtxFormat::Color) > 0) {
            elem[pos].Stream = 0;
            elem[pos].Offset = offset;
            elem[pos].Type = izanagi::graph::E_GRAPH_VTX_DECL_TYPE_COLOR;
            elem[pos].Usage = izanagi::graph::E_GRAPH_VTX_DECL_USAGE_COLOR;
            elem[pos].UsageIndex = 0;

            pos++;
            offset += sizeof(uint8_t) * 4;
        }
        if ((fmt & VtxFormat::Normal) > 0) {
            elem[pos].Stream = 0;
            elem[pos].Offset = offset;
            elem[pos].Type = izanagi::graph::E_GRAPH_VTX_DECL_TYPE_FLOAT3;
            elem[pos].Usage = izanagi::graph::E_GRAPH_VTX_DECL_USAGE_NORMAL;
            elem[pos].UsageIndex = 0;

            pos++;
            offset += sizeof(float) * 3;
        }

        return pos;
    }
}

PointDataGroup::Result PointDataGroup::read(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    const char* path)
{
    SPCDReader reader;

    if (!reader.open(path)) {
        return Result::NoExist;
    }

    const auto& header = reader.getHeader();

    if (m_vtxCnt + header.vtxNum > m_maxVtxNum) {
        return Result::Overflow;
    }

    if (m_format == 0) {
        m_format = header.vtxFormat;
    }
    else if (m_format != header.vtxFormat) {
        return Result::NotSameFormat;
    }

    m_vtxSize = computeVtxSize(m_format);

    AABB aabb;
    aabb.set(
        izanagi::math::CVector3(header.aabbMin[0], header.aabbMin[1], header.aabbMin[2]),
        izanagi::math::CVector3(header.aabbMax[0], header.aabbMax[1], header.aabbMax[2]));

    m_aabbs.push_back(aabb);

    // VD.
    if (!m_vd)
    {
        izanagi::graph::SVertexElement elems[4];

        auto num = makeVtxElement(m_format, elems);

        m_vd = device->CreateVertexDeclaration(elems, num);
    }

    // VB.
    if (!m_vbDynamicStream)
    {
        m_vbDynamicStream = device->CreateVertexBuffer(
            m_vtxSize, // 描画の際に使われるので、正しい値を設定しておく.
            0,       // glBufferStorage で確保されるので、この値はゼロにしておく.
            izanagi::graph::E_GRAPH_RSC_USAGE_DYNAMIC);

        CALL_GL_API(::glGenBuffers(1, &m_glVB));

        m_vbDynamicStream->overrideNativeResource(&m_glVB);

        CALL_GL_API(::glBindBuffer(GL_ARRAY_BUFFER, m_glVB));

        const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        auto bufferSize = m_vtxSize * m_maxVtxNum;

        CALL_GL_API(::glBufferStorage(
            GL_ARRAY_BUFFER,
            bufferSize,
            NULL,
            flags));

        m_mappedDataPtr = ::glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, flags);
        IZ_ASSERT(m_mappedDataPtr);
    }

    uint8_t* buffer = (uint8_t*)m_mappedDataPtr;
    uint32_t offset = m_vtxSize * m_vtxCnt;

    buffer += offset;

    reader.read((void**)&buffer, m_vtxSize * header.vtxNum, 1);

    m_vtxCnt += header.vtxNum;

    m_scale = header.scale;

    return Result::Succeeded;
}

void PointDataGroup::traverseAABB(std::function<void(const izanagi::math::SMatrix44&)> func)
{
    for each (const auto& aabb in m_aabbs)
    {
        const auto center = aabb.getCenter();
        auto size = aabb.getSize();

        izanagi::math::CMatrix44 mtxL2W;
        mtxL2W.SetScale(size.x, size.y, size.z);
        mtxL2W.Trans(center.x, center.y, center.z);

        func(mtxL2W);
    }
}
