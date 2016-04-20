#include "DynamicStreamMultiDrawApp.h"

////////////////////////////////////////////////////////////

void VertexInput::init()
{
    static const IZ_COLOR colors[] = {
        IZ_COLOR_RGBA(0xff, 0, 0, 0xff),
        IZ_COLOR_RGBA(0, 0xff, 0, 0xff),
        IZ_COLOR_RGBA(0, 0, 0xff, 0xff),
    };

    for (IZ_UINT n = 0; n < POINT_NUM; n++) {
        m_vtx[n].pos[0] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
        m_vtx[n].pos[1] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;
        m_vtx[n].pos[2] = izanagi::math::CMathRand::GetRandFloat() * 100.0f;

        m_vtx[n].pos[3] = 1.0f;

        m_vtx[n].color = colors[n % COUNTOF(colors)];
    }
}

IZ_BOOL VertexInput::needUpdate()
{
    auto ret = m_needUpdate;
    m_needUpdate = IZ_FALSE;
    return ret;
}

IZ_BOOL VertexInput::canWrite(IZ_UINT size)
{
    auto writeSize = getSize();
    return writeSize <= size;
}

IZ_UINT VertexInput::writeToBuffer(void* buffer, IZ_UINT offset)
{
    uint8_t* dst = (uint8_t*)buffer;
    dst += offset;

    auto size = getSize();
    void* src = &m_vtx[0];

    memcpy(dst, src, size);

    izanagi::sys::CThread::Sleep(100);

    m_offset = offset;

    return size;
}

IZ_UINT VertexInput::getSize() const
{
    return POINT_NUM * sizeof(Vertex);
}

IZ_UINT VertexInput::getCount() const
{
    return POINT_NUM;
}

void VertexInput::needUpdateForcibly()
{
    m_needUpdate = IZ_TRUE;
    m_cancel = true;
}

////////////////////////////////////////////////////////////

DynamicStreamMultiDrawApp::DynamicStreamMultiDrawApp()
{
}

DynamicStreamMultiDrawApp::~DynamicStreamMultiDrawApp()
{
}

// 初期化.
IZ_BOOL DynamicStreamMultiDrawApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    m_vtxStream.init(
        allocator,
        device,
        sizeof(Vertex),
        LIST_NUM * POINT_NUM);

    createVertices();

    IZ_BOOL result = IZ_TRUE;

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

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

void DynamicStreamMultiDrawApp::createVertices()
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    //izanagi::math::CMathRand::Init(st.wMilliseconds);
    izanagi::math::CMathRand::Init(0);

#ifdef MANUAL_ADD
    for (IZ_UINT i = 0; i < LIST_NUM; i++) {
        m_vtxInput[i].init();
    }
#else
#if 1
    m_vtxStream.beginAddInput();

    for (IZ_UINT i = 0; i < LIST_NUM; i++) {
        m_vtxInput[i].init();

        m_vtxStream.addInput(&m_vtxInput[i]);
    }

    m_vtxStream.endAddInput();
#else
    for (IZ_UINT i = 0; i < LIST_NUM; i++) {
        m_vtxInput[i].init();

        m_vtxStream.addInputSafely(&m_vtxInput[i]);
    }
#endif
#endif
}

// 解放.
void DynamicStreamMultiDrawApp::ReleaseInternal()
{    
    SAFE_RELEASE(m_vd);

    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_ps);
    SAFE_RELEASE(m_shd);

    m_vtxStream.terminate();
}

// 更新.
void DynamicStreamMultiDrawApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    auto& camera = GetCamera();
    
    camera.Update();

#ifdef MANUAL_ADD
    if (m_needUpdate) {
        if (m_curRegisteredNum < LIST_NUM) {
            for (IZ_UINT i = m_curRegisteredNum; i < 10; i++) {
                m_vtxStream.addInputSafely(&m_vtxInput[i]);
            }
            m_curRegisteredNum += 10;
        }
    }
#else
    if (m_needUpdate) {
        IZ_PRINTF("****** Force Update\n");
        for (IZ_UINT i = 0; i < LIST_NUM; i++) {
            m_vtxInput[i].needUpdateForcibly();
        }
        m_vtxStream.notifyUpdateForcibly();
        m_needUpdate = IZ_FALSE;
    }
    if (m_needCancel) {
        IZ_PRINTF("****** Force Cancel\n");

        m_vtxStream.beginRemoveInput();
        for (IZ_UINT i = 0; i < LIST_NUM; i++) {
            m_vtxInput[i].cancel();
        }
        m_vtxStream.endRemoveInput();

        m_vtxStream.notifyUpdateForcibly(IZ_FALSE);
        m_needCancel = IZ_FALSE;
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
void DynamicStreamMultiDrawApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    RenderDynamicStream(device);
}

void DynamicStreamMultiDrawApp::RenderDynamicStream(izanagi::graph::CGraphicsDevice* device)
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

    auto vb = m_vtxStream.getVB();

    device->SetVertexBuffer(0, 0, sizeof(Vertex), vb);
    device->SetVertexDeclaration(m_vd);

    IZ_UINT offset = 0;
    IZ_UINT length = LIST_NUM * sizeof(Vertex) * POINT_NUM;

    // NOTE
    // https://shikihuiku.wordpress.com/2014/02/13/%E6%9C%80%E8%BF%91%E3%81%AEopengl%E3%81%AB%E3%81%8A%E3%81%91%E3%82%8B%E5%8A%B9%E7%8E%87%E7%9A%84%E3%81%AAdrawcall%E3%82%92%E8%80%83%E3%81%88%E3%82%8B/
    // https://www.opengl.org/wiki/GLAPI/glMultiDrawArraysIndirect
    // https://www.reddit.com/r/opengl/comments/3m9u36/how_to_render_using_glmultidrawarraysindirect/

    izanagi::sys::CTimer timer;
    timer.Begin();

    auto cmdData = m_vtxStream.getCommands();

    auto cmd = std::get<0>(cmdData);
    auto num = std::get<1>(cmdData);

    if (cmd) {
        device->DrawPrimitive(
            izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
            [&](GLenum mode) {
            glMultiDrawArraysIndirect(mode, cmd, num, 0);
        });
    }

    auto time = timer.End();
    IZ_PRINTF("Time [%f]\n", time);
}

IZ_BOOL DynamicStreamMultiDrawApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    if (key == izanagi::sys::E_KEYBOARD_BUTTON_RETURN) {
        m_needUpdate = IZ_TRUE;
    }
    else if (key == izanagi::sys::E_KEYBOARD_BUTTON_SPACE) {
        m_needCancel = IZ_TRUE;
    }
    return IZ_TRUE;
}
