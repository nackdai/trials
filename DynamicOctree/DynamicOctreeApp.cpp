#include "DynamicOctreeApp.h"
#include "DynamicOctreeRenderer.h"

////////////////////////////////////////////////

class PointObj : public DynamicOctreeObject {
public:
    PointObj() {}
    virtual ~PointObj() {}

    virtual izanagi::math::SVector4 getCenter() override
    {
        izanagi::math::CVector4 ret(vtx.pos[0], vtx.pos[1], vtx.pos[2]);
        return std::move(ret);
    }

    Vertex vtx;
};

////////////////////////////////////////////////

DynamicOctreeApp::DynamicOctreeApp()
{
}

DynamicOctreeApp::~DynamicOctreeApp()
{
}

// 初期化.
IZ_BOOL DynamicOctreeApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    SYSTEMTIME st;
    GetSystemTime(&st);

    izanagi::math::CMathRand::Init(st.wMilliseconds);
    //izanagi::math::CMathRand::Init(0);

    // Vertex Buffer.
    createVBForDynamicStream(allocator, device);

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

    m_octree.init(
        10.0f,
        //izanagi::math::CVector4(5.0f, 5.0f, 5.0f),
        izanagi::math::CVector4(0.0f, 0.0f, 0.0f),
        1.0f,
        4);

    DynamicOctreeRenderer::instance().init(device, allocator);

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

void DynamicOctreeApp::createVBForDynamicStream(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device)
{
    m_vbDynamicStream = device->CreateVertexBuffer(
        sizeof(Vertex), // 描画の際に使われるので、正しい値を設定しておく.
        0,              // glBufferStorage で確保されるので、この値はゼロにしておく.
        izanagi::graph::E_GRAPH_RSC_USAGE_DYNAMIC);

    CALL_GL_API(::glGenBuffers(1, &m_glVB));

    m_vbDynamicStream->overrideNativeResource(&m_glVB);

    CALL_GL_API(::glBindBuffer(GL_ARRAY_BUFFER, m_glVB));

    const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    m_bufferSize = sizeof(Vertex) * POINT_NUM;

    CALL_GL_API(::glBufferStorage(
        GL_ARRAY_BUFFER,
        m_bufferSize,
        NULL,
        flags));

    m_mappedDataPtr = ::glMapBufferRange(GL_ARRAY_BUFFER, 0, m_bufferSize, flags);
    IZ_ASSERT(m_mappedDataPtr);
}

// 解放.
void DynamicOctreeApp::ReleaseInternal()
{
    m_vbDynamicStream->overrideNativeResource(nullptr);
    CALL_GL_API(::glDeleteBuffers(1, &m_glVB));

    SAFE_RELEASE(m_vbDynamicStream);

    SAFE_RELEASE(m_vd);

    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_ps);
    SAFE_RELEASE(m_shd);

    SAFE_RELEASE(m_basicShd);
    m_octree.clear();

    DynamicOctreeRenderer::instance().terminate();
}

// 更新.
void DynamicOctreeApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    auto& camera = GetCamera();
    
    camera.Update();

    if (m_willMerge) {
        m_octree.merge(2);
        m_willMerge = IZ_FALSE;
    }
    else if (m_addPoint) {
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
            PointObj* vtx = new PointObj();
            {
#if 1
                vtx->vtx.pos[0] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
                vtx->vtx.pos[1] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
                vtx->vtx.pos[2] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
#else
                vtx->vtx.pos[0] = 42.1749191f;
                vtx->vtx.pos[1] = 13.1274967f;
                vtx->vtx.pos[2] = 0.126667321f;
                //vtx->vtx.pos[0] = 15.0f * (m_vtx.size() + 1);
                //vtx->vtx.pos[1] = 0.0f;
                //vtx->vtx.pos[2] = 0.0f;
#endif

                vtx->vtx.pos[3] = 1.0f;
            }

            auto depth = m_octree.add(vtx);
            IZ_PRINTF("Depth[%d]\n", depth);

            vtx->vtx.color = colors[depth % COUNTOF(colors)];

            m_vtx.push_back(vtx->vtx);
        }

        m_addPoint = IZ_FALSE;
    }
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
void DynamicOctreeApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
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

    auto node = m_octree.getRootNode();

    if (m_basicShd->Begin(device, 0, IZ_FALSE)) {

        if (m_basicShd->BeginPass(0)) {
            _SetShaderParam(m_basicShd, "g_mW2C", (const void*)&mtxW2C, sizeof(mtxW2C));

            DynamicOctreeRenderer::instance().renderNodeRecursivly(
                device, node,
                [&](const izanagi::math::SMatrix44& mtxL2W) {
                _SetShaderParam(m_basicShd, "g_mL2W", (const void*)&mtxL2W, sizeof(mtxL2W));

                m_basicShd->CommitChanges(device);
            });

            m_basicShd->EndPass();
        }

        m_basicShd->End(device);
    }
}

IZ_BOOL DynamicOctreeApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    if (key == izanagi::sys::E_KEYBOARD_BUTTON_RETURN) {
        m_addPoint = IZ_TRUE;
    }
    else if (key == izanagi::sys::E_KEYBOARD_BUTTON_SPACE) {
        m_willMerge = IZ_TRUE;
    }
    return IZ_TRUE;
}
