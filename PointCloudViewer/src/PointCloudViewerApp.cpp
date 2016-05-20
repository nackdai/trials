#include "PointCloudViewerApp.h"

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

#ifdef ENABLE_MANUAL
#else
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
#endif

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

            auto found = std::find(m_files.begin(), m_files.end(), path);

            if (found == m_files.end()) {
                m_files.push_back(path);
            }
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

    SAFE_RELEASE(m_wiredBox);

    for each (auto group in m_groups) {
        delete group;
    }
}

// 更新.
void PointCloudViewerApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    auto& camera = GetCamera();
    
    camera.Update();

#ifdef ENABLE_MANUAL
    if (m_readData && !m_files.empty()) {
        if (!m_curGroup) {
            auto group = new PointDataGroup(100000);
            m_groups.push_back(group);

            m_curGroup = group;
        }

        auto it = m_files.begin();
        auto& file = *it;

        {
            IZ_PRINTF("%s\n", file.c_str());
            auto res = m_curGroup->read(m_allocator, device, file.c_str());

            if (PointDataGroup::needOtherGroup(res)) {
                auto group = new PointDataGroup(100000);
                m_groups.push_back(group);

                res = group->read(m_allocator, device, file.c_str());
                IZ_ASSERT(PointDataGroup::isSucceeded(res));

                m_curGroup = group;
            }
        }

        m_files.erase(it);

        m_total = 0;
        for each (auto g in m_groups) {
            m_total += g->getVtxNum();
        }
    }
    m_readData = false;
#endif
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

    if (m_groups.size() > 0) {
        IZ_FLOAT scale = m_groups[0]->getScale();
        auto hScale = m_shd->GetHandleByName("scale");
        m_shd->SetFloat(device, hScale, scale);
    }

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

    auto font = GetDebugFont();
    if (device->Begin2D()) {
        font->Begin(device);
        font->DBPrint(
            device,
            0, 40,
            "num : [%d]", m_total);
        font->End();
        device->End2D();
    }
}

IZ_BOOL PointCloudViewerApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    if (key == izanagi::sys::E_KEYBOARD_BUTTON_RETURN) {
        m_readData = true;
    }

    return IZ_TRUE;
}
