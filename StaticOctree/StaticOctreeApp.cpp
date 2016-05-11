#include "StaticOctreeApp.h"

#include "XyzReader.h"

StaticOctreeApp::StaticOctreeApp()
{
}

StaticOctreeApp::~StaticOctreeApp()
{
}

// 初期化.
IZ_BOOL StaticOctreeApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    SYSTEMTIME st;
    GetSystemTime(&st);

    izanagi::math::CMathRand::Init(st.wMilliseconds);
    //izanagi::math::CMathRand::Init(0);

    m_octree.initialize(
        allocator,
        izanagi::math::CVector4(-200.0f, -200.0f, -200.0f),
        //izanagi::math::CVector4(0.0f, 0.0f, 0.0f),
        izanagi::math::CVector4(200.0f, 200.0f, 200.0f),
        3);

#ifdef ENABLE_MANUAL
    // Vertex Buffer.
    createVBForDynamicStream(allocator, device, POINT_NUM);
#else
    XyzReader reader;
    reader.open(std::string("bunny.xyz"));

    XyzReader::Vertex vtx;

    static const IZ_COLOR colors[] = {
        izanagi::CColor::RED,
        izanagi::CColor::BLUE,
        izanagi::CColor::GREEN,
        izanagi::CColor::ORANGE,
        izanagi::CColor::DEEPPINK,
        izanagi::CColor::PURPLE,
        izanagi::CColor::VIOLET,
    };

    IZ_UINT cnt = 0;

    while (reader.readVtx(vtx)) {
        Vertex _vtx;

        _vtx.pos[0] = vtx.pos[0] * 1000.0f;
        _vtx.pos[1] = vtx.pos[1] * 1000.0f;
        _vtx.pos[2] = vtx.pos[2] * 1000.0f;
        _vtx.pos[3] = 1.0f;

        auto root = m_octree.getNode(0);
        auto nodes = m_octree.getChildren(root);

        bool runLoop = true;

        while (runLoop) {
            bool find = false;

            for (IZ_UINT i = 0; i < 8; i++) {
                auto node = nodes[i];

                if (node->isContain(_vtx)) {
                    find = true;

                    nodes = m_octree.getChildren(node);

                    if (!nodes) {
                        auto depth = node->getLevel();
                        _vtx.color = colors[depth % COUNTOF(colors)];

                        node->add(_vtx);
                        runLoop = false;
                    }

                    break;
                }
            }

            IZ_ASSERT(find);
        }

        m_vtx.push_back(_vtx);

        cnt++;
    }

    reader.close();

    createVBForDynamicStream(allocator, device, cnt);
#endif

    {
        izanagi::graph::SVertexElement elems[] = {
            { 0, 0, izanagi::graph::E_GRAPH_VTX_DECL_TYPE_FLOAT4, izanagi::graph::E_GRAPH_VTX_DECL_USAGE_POSITION, 0 },
            { 0, 16, izanagi::graph::E_GRAPH_VTX_DECL_TYPE_COLOR, izanagi::graph::E_GRAPH_VTX_DECL_USAGE_COLOR, 0 },
        };

        m_vd = device->CreateVertexDeclaration(elems, COUNTOF(elems));
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

void StaticOctreeApp::createVBForDynamicStream(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    IZ_UINT vtxNum)
{
    m_vbDynamicStream = device->CreateVertexBuffer(
        sizeof(Vertex), // 描画の際に使われるので、正しい値を設定しておく.
        0,              // glBufferStorage で確保されるので、この値はゼロにしておく.
        izanagi::graph::E_GRAPH_RSC_USAGE_DYNAMIC);

    CALL_GL_API(::glGenBuffers(1, &m_glVB));

    m_vbDynamicStream->overrideNativeResource(&m_glVB);

    CALL_GL_API(::glBindBuffer(GL_ARRAY_BUFFER, m_glVB));

    const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    m_bufferSize = sizeof(Vertex) * vtxNum;

    CALL_GL_API(::glBufferStorage(
        GL_ARRAY_BUFFER,
        m_bufferSize,
        NULL,
        flags));

    m_mappedDataPtr = ::glMapBufferRange(GL_ARRAY_BUFFER, 0, m_bufferSize, flags);
    IZ_ASSERT(m_mappedDataPtr);
}

// 解放.
void StaticOctreeApp::ReleaseInternal()
{
    m_vbDynamicStream->overrideNativeResource(nullptr);
    CALL_GL_API(::glDeleteBuffers(1, &m_glVB));

    SAFE_RELEASE(m_vbDynamicStream);

    SAFE_RELEASE(m_vd);

    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_ps);
    SAFE_RELEASE(m_shd);

    SAFE_RELEASE(m_basicShd);

    SAFE_RELEASE(m_wiredBox);

    m_octree.clear();
}

// 更新.
void StaticOctreeApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    auto& camera = GetCamera();
    
    camera.Update();

#ifdef ENABLE_MANUAL
    if (m_addPoint) {
        static const IZ_COLOR colors[] = {
            izanagi::CColor::RED,
            izanagi::CColor::BLUE,
            izanagi::CColor::GREEN,
            izanagi::CColor::ORANGE,
            izanagi::CColor::DEEPPINK,
            izanagi::CColor::PURPLE,
            izanagi::CColor::VIOLET,
        };

        if (m_vtx.size() == POINT_NUM) {
            return;
        }

        //for (IZ_UINT i = 0; i < 1000; i++) {
        for (IZ_UINT i = 0; i < 10; i++) {
            Vertex vtx;
            {
                vtx.pos[0] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
                vtx.pos[1] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
                vtx.pos[2] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
                vtx.pos[3] = 1.0f;
            }

            auto root = m_octree.getNode(0);
            auto nodes = m_octree.getChildren(root);

            bool runLoop = true;

            while (runLoop) {
                for (IZ_UINT i = 0; i < 8; i++) {
                    auto node = nodes[i];

                    if (node->isContain(vtx)) {
                        nodes = m_octree.getChildren(node);

                        if (!nodes) {
                            auto depth = node->getLevel();
                            vtx.color = colors[depth % COUNTOF(colors)];

                            node->add(vtx);
                            runLoop = false;
                        }

                        break;
                    }
                }
            }

            m_vtx.push_back(vtx);
        }

        m_addPoint = IZ_FALSE;
    }
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
void StaticOctreeApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
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

    device->SetVertexBuffer(0, 0, sizeof(Vertex), m_vbDynamicStream);
    device->SetVertexDeclaration(m_vd);

    auto num = m_vtx.size();
    if (num > 0) {
        memcpy(m_mappedDataPtr, &m_vtx[0], sizeof(Vertex) * num);

        device->DrawPrimitive(
            izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
            0,
            num);
    }

    if (m_basicShd->Begin(device, 0, IZ_FALSE))
    {
        if (m_basicShd->BeginPass(0))
        {
            _SetShaderParam(m_basicShd, "g_mW2C", (const void*)&mtxW2C, sizeof(mtxW2C));

            std::vector<Node*> nodes;

            m_octree.traverse(
                [&](Node* node) {
                izanagi::col::Octree<Node>::RetvalTraverse ret(IZ_FALSE, IZ_TRUE);

                //if (node->hasVtx())
                {
                    izanagi::col::AABB aabb;
                    node->getAABB(aabb);

                    auto center = aabb.getCenter();
                    auto size = aabb.getSize();

                    izanagi::math::CMatrix44 mtxL2W;
                    mtxL2W.SetScale(size.x, size.y, size.z);
                    mtxL2W.Trans(center.x, center.y, center.z);

                    _SetShaderParam(m_basicShd, "g_mL2W", (const void*)&mtxL2W, sizeof(mtxL2W));

                    m_basicShd->CommitChanges(device);

                    m_wiredBox->Draw(device);
                }

                return ret;
            }, nodes, IZ_FALSE);

            m_basicShd->EndPass();
        }

        m_basicShd->End(device);
    }
}

IZ_BOOL StaticOctreeApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    if (key == izanagi::sys::E_KEYBOARD_BUTTON_RETURN) {
        m_addPoint = IZ_TRUE;
    }
    return IZ_TRUE;
}
