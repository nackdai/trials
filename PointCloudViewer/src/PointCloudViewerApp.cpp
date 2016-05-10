#include "PointCloudViewerApp.h"
#include "SPCDReader.h"

////////////////////////////////////////////////////

namespace {
    uint32_t computeVtxSize(uint32_t fmt)
    {
        uint32_t size = 0;

        if ((fmt & VtxFormat::Position) > 0) {
            size += sizeof(float) * 3;
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
            elem[pos].Type = izanagi::graph::E_GRAPH_VTX_DECL_TYPE_FLOAT3;
            elem[pos].Usage = izanagi::graph::E_GRAPH_VTX_DECL_USAGE_POSITION;
            elem[pos].UsageIndex = 0;

            pos++;
            offset += sizeof(float) * 3;
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

////////////////////////////////////////////////////

PointCloudViewerApp::PointCloudViewerApp()
{
}

PointCloudViewerApp::~PointCloudViewerApp()
{
}

// 初期化.
IZ_BOOL PointCloudViewerApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    searchDir(".\\*");

    PointDataGroup* group = new PointDataGroup(15000);
    m_groups.push_back(group);

    for each (auto& file in m_files)
    {
        auto res = group->read(allocator, device, file.c_str());

#if 1
        if (PointDataGroup::needOtherGroup(res)) {
            group = new PointDataGroup(15000);
            m_groups.push_back(group);

            res = group->read(allocator, device, file.c_str());
            IZ_ASSERT(PointDataGroup::isSucceeded(res));
        }
#else
        break;
#endif
    }

    {
        izanagi::CFileInputStream in;
        in.Open("data/BasicShader.shd");

        m_basicShd = izanagi::shader::CShaderBasic::CreateShader<izanagi::shader::CShaderBasic>(
            allocator,
            device,
            &in);
    }

    {
        {
            izanagi::CFileInputStream in;
            in.Open("shader/vs.glsl");

            std::vector<IZ_BYTE> buf(in.GetSize() + 1);
            in.Read(&buf[0], 0, buf.size());

            buf[buf.size() - 1] = 0;

            m_vs = device->CreateVertexShader(&buf[0]);
        }

        {
            izanagi::CFileInputStream in;
            in.Open("shader/ps.glsl");

            std::vector<IZ_BYTE> buf(in.GetSize() + 1);
            in.Read(&buf[0], 0, buf.size());

            buf[buf.size() - 1] = 0;

            m_ps = device->CreatePixelShader(&buf[0]);
        }

        m_shd = device->CreateShaderProgram();
        m_shd->AttachVertexShader(m_vs);
        m_shd->AttachPixelShader(m_ps);
    }

    {
        m_wiredBox = izanagi::CDebugMeshWiredBox::create(
            allocator,
            device,
            IZ_COLOR_RGBA(0xff, 0xff, 0x00, 0xff),
            1.0f, 1.0f, 1.0f);
        IZ_ASSERT(m_wiredBox);
    }

    // カメラ
    camera.Init(
        izanagi::math::CVector4(0.0f, 0.0f, 30.0f, 1.0f),
        izanagi::math::CVector4(0.0f, 0.0f, 0.0f, 1.0f),
        izanagi::math::CVector4(0.0f, 1.0f, 0.0f, 1.0f),
        1.0f,
        10000.0f,
        izanagi::math::CMath::Deg2Rad(60.0f),
        (IZ_FLOAT)device->GetBackBufferWidth() / device->GetBackBufferHeight());
    camera.Update();

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

void PointCloudViewerApp::searchDir(const char* filename)
{
    static const char* BASE_DIR = ".\\";
    static const char* EXT = ".spcd";

    HANDLE hFind;
    WIN32_FIND_DATA fd;

    hFind = FindFirstFile(filename, &fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        std::string file(fd.cFileName);

#if 0
        if (file.compare("SampleRunner") == 0
            || file.compare("SampleRunner.exe") == 0
            || file.compare("Bin") == 0)
        {
            continue;
        }
#endif

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (file.compare(".") == 0 || file.compare("..") == 0) {
                continue;
            }

            std::string path(BASE_DIR);
            path += file.c_str();
            path += "\\*";
            path += EXT;

            searchDir(path.c_str());
        }
        else if (file.find(EXT, 0) != std::string::npos) {
            std::string path(BASE_DIR);
            path += file.c_str();

            m_files.push_back(path);
        }
    } while (FindNextFile(hFind, &fd));
}

// 解放.
void PointCloudViewerApp::ReleaseInternal()
{
    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_ps);
    SAFE_RELEASE(m_shd);

    SAFE_RELEASE(m_basicShd);
}

// 更新.
void PointCloudViewerApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    auto& camera = GetCamera();
    
    camera.Update();
}

namespace {
    inline void _SetShaderParam(
        izanagi::shader::CShaderBasic* shader,
        const char* name,
        const void* value,
        IZ_UINT bytes)
    {
        izanagi::shader::IZ_SHADER_HANDLE handle = shader->GetParameterByName(name);
        IZ_ASSERT(handle != 0);

        shader->SetParamValue(
            handle,
            value,
            bytes);
    }
}

// 描画.
void PointCloudViewerApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    CALL_GL_API(::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE));
    CALL_GL_API(::glEnable(GL_POINT_SPRITE));

    device->SetShaderProgram(m_shd);

    auto& camera = GetCamera();

    izanagi::math::SMatrix44 mtxW2C;
    izanagi::math::SMatrix44::Copy(mtxW2C, camera.GetParam().mtxW2C);

    IZ_FLOAT pointSize = 5.0f;
    auto hSize = m_shd->GetHandleByName("size");
    m_shd->SetFloat(device, hSize, pointSize);

    auto hMtxW2C = m_shd->GetHandleByName("mtxW2C");
    m_shd->SetMatrixArrayAsVectorArray(device, hMtxW2C, &mtxW2C, 4);

    for each (auto group in m_groups)
    {
        auto vd = group->getVD();
        auto vb = group->getVB();
        auto vtxSize = group->getVtxSize();
        auto num = group->getVtxNum();

        device->SetVertexBuffer(0, 0, vtxSize, vb);
        device->SetVertexDeclaration(vd);

        device->DrawPrimitive(
            izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
            0,
            num);
    }

    if (m_basicShd->Begin(device, 0, IZ_FALSE)) {

        if (m_basicShd->BeginPass(0)) {
            _SetShaderParam(m_basicShd, "g_mW2C", (const void*)&mtxW2C, sizeof(mtxW2C));

            for each (auto group in m_groups)
            {
                group->traverseAABB(
                    [&](const izanagi::math::SMatrix44& mtxL2W) {
                    _SetShaderParam(m_basicShd, "g_mL2W", (const void*)&mtxL2W, sizeof(mtxL2W));

                    m_basicShd->CommitChanges(device);

                    m_wiredBox->Draw(device);
                });
            }

            m_basicShd->EndPass();
        }

        m_basicShd->End(device);
    }
}

IZ_BOOL PointCloudViewerApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
