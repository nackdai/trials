#include "DynamicStreamApp.h"

DynamicStreamApp::DynamicStreamApp()
{
}

DynamicStreamApp::~DynamicStreamApp()
{
}

// 初期化.
IZ_BOOL DynamicStreamApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    // Vertex Buffer.
    createVBForDynamicStream(allocator, device);
    createVBForMapUnmap(allocator, device);

    {
        izanagi::graph::SVertexElement elems[] = {
            { 0, 0, izanagi::graph::E_GRAPH_VTX_DECL_TYPE_FLOAT4, izanagi::graph::E_GRAPH_VTX_DECL_USAGE_POSITION, 0 },
            { 0, 16, izanagi::graph::E_GRAPH_VTX_DECL_TYPE_COLOR, izanagi::graph::E_GRAPH_VTX_DECL_USAGE_COLOR, 0 },
        };

        m_vd = device->CreateVertexDeclaration(elems, COUNTOF(elems));
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
        500.0f,
        izanagi::math::CMath::Deg2Rad(60.0f),
        (IZ_FLOAT)device->GetBackBufferWidth() / device->GetBackBufferHeight());
    camera.Update();

    createVertices();

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

void DynamicStreamApp::createVBForDynamicStream(
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
    m_bufferSize = LIST_NUM * sizeof(Vertex) * POINT_NUM;

    CALL_GL_API(::glBufferStorage(
        GL_ARRAY_BUFFER,
        m_bufferSize,
        NULL,
        flags));

    m_mappedDataPtr = ::glMapBufferRange(GL_ARRAY_BUFFER, 0, m_bufferSize, flags);
    IZ_ASSERT(m_mappedDataPtr);
}

void DynamicStreamApp::createVBForMapUnmap(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device)
{
    m_vbMapUnmap = device->CreateVertexBuffer(
        sizeof(Vertex),
        POINT_NUM,
        izanagi::graph::E_GRAPH_RSC_USAGE_DYNAMIC);
}

void DynamicStreamApp::createVertices()
{
    static const IZ_COLOR colors[] = {
        IZ_COLOR_RGBA(0xff, 0, 0, 0xff),
        IZ_COLOR_RGBA(0, 0xff, 0, 0xff),
        IZ_COLOR_RGBA(0, 0, 0xff, 0xff),
    };

    SYSTEMTIME st;
    GetSystemTime(&st);

    //izanagi::math::CMathRand::Init(st.wMilliseconds);
    izanagi::math::CMathRand::Init(0);

    IZ_UINT offset = 0;

    for (IZ_UINT i = 0; i < LIST_NUM; i++) {
        for (IZ_UINT n = 0; n < POINT_NUM; n++) {
            m_vtx[i][n].pos[0] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
            m_vtx[i][n].pos[1] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
            m_vtx[i][n].pos[2] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;

            m_vtx[i][n].pos[3] = 1.0f;

            m_vtx[i][n].color = colors[n % COUNTOF(colors)];
        }

#ifdef ENABLE_TRHEAD
        m_items[i].offset = offset;
        offset += POINT_NUM;
#endif
    }

#ifdef ENABLE_TRHEAD
    m_thread = std::thread([&] {
        while (m_isRunThread) {
            for (IZ_UINT i = 0; i < LIST_NUM; i++) {
                if (!m_items[i].isRenderable && m_mappedDataPtr) {
                    IZ_UINT8* dst = (IZ_UINT8*)m_mappedDataPtr;
                    dst += m_items[i].offset * sizeof(Vertex);

                    memcpy(dst, m_vtx[i], sizeof(m_vtx[i]));

                    m_items[i].isRenderable = true;
                }
            }
        }
    });
#endif
}

// 解放.
void DynamicStreamApp::ReleaseInternal()
{
#ifdef ENABLE_TRHEAD
    m_isRunThread = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
#endif

    m_vbDynamicStream->overrideNativeResource(nullptr);
    CALL_GL_API(::glDeleteBuffers(1, &m_glVB));

    SAFE_RELEASE(m_vbDynamicStream);
    SAFE_RELEASE(m_vbMapUnmap);

    SAFE_RELEASE(m_vd);

    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_ps);
    SAFE_RELEASE(m_shd);
}

// 更新.
void DynamicStreamApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
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
void DynamicStreamApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    RenderDynamicStream(device);
    //RenderMapUnmap(device);
}

void DynamicStreamApp::RenderDynamicStream(izanagi::graph::CGraphicsDevice* device)
{
    CALL_GL_API(::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE));
    CALL_GL_API(::glEnable(GL_POINT_SPRITE));

    device->SetShaderProgram(m_shd);

    auto& camera = GetCamera();

    izanagi::math::SMatrix44 mtxW2C;
    izanagi::math::SMatrix44::Copy(mtxW2C, camera.GetParam().mtxW2C);

    IZ_FLOAT pointSize = 100.0f;
    auto hSize = m_shd->GetHandleByName("size");
    m_shd->SetFloat(device, hSize, pointSize);

    auto hMtxW2C = m_shd->GetHandleByName("mtxW2C");
    m_shd->SetMatrixArrayAsVectorArray(device, hMtxW2C, &mtxW2C, 4);

    device->SetVertexBuffer(0, 0, sizeof(Vertex), m_vbDynamicStream);
    device->SetVertexDeclaration(m_vd);

    IZ_UINT offset = 0;
    IZ_UINT length = LIST_NUM * sizeof(Vertex) * POINT_NUM;

    // Need to wait for this area to become available. If we've sized things properly, it will always be 
    // available right away.
    //m_mgrBufferLock.WaitForLockedRange(offset, length);

    for (IZ_UINT i = 0; i < LIST_NUM; i++) {
#ifdef ENABLE_TRHEAD
        if (m_items[i].isRenderable) {
            device->DrawPrimitive(
                izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
                m_items[i].offset,
                POINT_NUM);

            m_items[i].isRenderable = false;
        }
#else
        IZ_UINT8* dst = (IZ_UINT8*)m_mappedDataPtr;
        dst += offset * sizeof(Vertex);

        memcpy(dst, m_vtx[i], sizeof(m_vtx[i]));

        device->DrawPrimitive(
            izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
            offset,
            POINT_NUM);

        offset += POINT_NUM;
#endif
    }

    //m_mgrBufferLock.LockRange(offset, length);
}

void DynamicStreamApp::RenderMapUnmap(izanagi::graph::CGraphicsDevice* device)
{
    CALL_GL_API(::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE));
    CALL_GL_API(::glEnable(GL_POINT_SPRITE));

    device->SetShaderProgram(m_shd);

    auto& camera = GetCamera();

    izanagi::math::SMatrix44 mtxW2C;
    izanagi::math::SMatrix44::Copy(mtxW2C, camera.GetParam().mtxW2C);

    IZ_FLOAT pointSize = 100.0f;
    auto hSize = m_shd->GetHandleByName("size");
    m_shd->SetFloat(device, hSize, pointSize);

    auto hMtxW2C = m_shd->GetHandleByName("mtxW2C");
    m_shd->SetMatrixArrayAsVectorArray(device, hMtxW2C, &mtxW2C, 4);

    device->SetVertexBuffer(0, 0, sizeof(Vertex), m_vbMapUnmap);
    device->SetVertexDeclaration(m_vd);

    for (IZ_UINT i = 0; i < LIST_NUM; i++) {
        void* dst = nullptr;
        m_vbMapUnmap->Lock(
            device,
            0,
            0,
            (void**)&dst,
            IZ_FALSE);

        memcpy(dst, m_vtx[i], sizeof(m_vtx[i]));

        m_vbMapUnmap->Unlock(device);

        device->DrawPrimitive(
            izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
            0,
            POINT_NUM);
    }
}


IZ_BOOL DynamicStreamApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
