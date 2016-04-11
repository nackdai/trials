#include "RenderPointSpriteApp.h"
#include "PlyReader.h"

RenderPointSpriteApp::RenderPointSpriteApp()
{
}

RenderPointSpriteApp::~RenderPointSpriteApp()
{
}

// 初期化.
IZ_BOOL RenderPointSpriteApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    {
        PlyReader reader;
        reader.open("data/bunny.ply");

        m_pointNum = reader.getVtxNum();

        m_vb = device->CreateVertexBuffer(
            sizeof(Vertex),
            m_pointNum,
            izanagi::graph::E_GRAPH_RSC_USAGE_STATIC);

        Vertex* vtx;
        auto pitch = m_vb->Lock(device, 0, 0, (void**)&vtx, IZ_FALSE);

        PlyReader::Vertex plyVtx;
        
        while (reader.readVtx(plyVtx)) {
            vtx->pos[0] = plyVtx.pos[0] * 100.0f;
            vtx->pos[1] = plyVtx.pos[1] * 100.0f;
            vtx->pos[2] = plyVtx.pos[2] * 100.0f;
            vtx->pos[3] = 1.0f;

            vtx->color = IZ_COLOR_RGBA(
                plyVtx.color[0],
                plyVtx.color[1],
                plyVtx.color[2],
                plyVtx.color[3]);

            vtx++;
        }

        m_vb->Unlock(device);
    }

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

    m_rt = device->CreateRenderTarget(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        //izanagi::graph::E_GRAPH_PIXEL_FMT_R32F);
        izanagi::graph::E_GRAPH_PIXEL_FMT_RGBA8);

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

// 解放.
void RenderPointSpriteApp::ReleaseInternal()
{
    SAFE_RELEASE(m_vb);
    SAFE_RELEASE(m_vd);

    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_ps);
    SAFE_RELEASE(m_shd);

    SAFE_RELEASE(m_rt);
}

// 更新.
void RenderPointSpriteApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
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
void RenderPointSpriteApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    CALL_GL_API(::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE));
    CALL_GL_API(::glEnable(GL_POINT_SPRITE));

#if 0
    device->BeginScene(
        &m_rt,
        1,
        izanagi::graph::E_GRAPH_CLEAR_FLAG_ALL,
        IZ_COLOR_RGBA(0xff, 0xff, 0xff, 0x00));
#endif

    device->SetShaderProgram(m_shd);

    auto& camera = GetCamera();

    const izanagi::math::SMatrix44& mtxW2V = camera.GetParam().mtxW2V;
    const izanagi::math::SMatrix44& mtxV2C = camera.GetParam().mtxV2C;
    auto farClip = camera.GetParam().cameraFar;

    izanagi::math::SMatrix44 mtxC2V;
    izanagi::math::SMatrix44::Inverse(mtxC2V, mtxV2C);

    izanagi::math::CVector4 screen(SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f);

    IZ_FLOAT pointSize = 0.5f;
    auto hSize = m_shd->GetHandleByName("size");
    m_shd->SetFloat(device, hSize, pointSize);

    auto hMtxW2V = m_shd->GetHandleByName("mtxW2V");
    m_shd->SetMatrixArrayAsVectorArray(device, hMtxW2V, &mtxW2V, 4);

    auto hMtxV2C = m_shd->GetHandleByName("mtxV2C");
    m_shd->SetMatrixArrayAsVectorArray(device, hMtxV2C, &mtxV2C, 4);

    auto hMtxC2V = m_shd->GetHandleByName("mtxC2V");
    m_shd->SetMatrixArrayAsVectorArray(device, hMtxC2V, &mtxC2V, 4);

    auto hFarClip = m_shd->GetHandleByName("farClip");
    m_shd->SetFloat(device, hFarClip, farClip);

    auto hScreen = m_shd->GetHandleByName("screen");
    m_shd->SetVector(device, hScreen, screen);

    device->SetVertexBuffer(0, 0, sizeof(Vertex), m_vb);
    device->SetVertexDeclaration(m_vd);

    device->DrawPrimitive(
        izanagi::graph::E_GRAPH_PRIM_TYPE_POINTLIST,
        0,
        m_pointNum);

#if 0
    device->EndScene();

    if (device->Begin2D()) {
        device->SetTexture(0, m_rt);

        device->Draw2DSprite(
            izanagi::CFloatRect(0.0f, 0.0f, 1.0f, 1.0f),
            izanagi::CIntRect(0, 100, 320, 180));

        device->End2D();
    }
#endif
}

IZ_BOOL RenderPointSpriteApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
