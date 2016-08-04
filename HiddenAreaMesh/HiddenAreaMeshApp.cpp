#include <vector>
#include "HiddenAreaMeshApp.h"

bool HiddenAreaMeshApp::Shader::Init(
    izanagi::graph::CGraphicsDevice* device,
    const char* pathVs, const char* pathPs)
{
    // シェーダ
    {
        izanagi::CFileInputStream in;
        VRETURN(in.Open(pathVs));

        std::vector<uint8_t> buf(in.GetSize() + 1);
        in.Read(&buf[0], 0, in.GetSize());

        m_vs = device->CreateVertexShader(&buf[0]);

        VRETURN(m_vs != IZ_NULL);
    }
    {
        izanagi::CFileInputStream in;
        VRETURN(in.Open(pathPs));

        std::vector<uint8_t> buf(in.GetSize() + 1);
        in.Read(&buf[0], 0, in.GetSize());

        m_fs = device->CreatePixelShader(&buf[0]);

        VRETURN(m_fs != IZ_NULL);
    }
    {
        m_shd = device->CreateShaderProgram();
        m_shd->AttachVertexShader(m_vs);
        m_shd->AttachPixelShader(m_fs);
    }

    return true;
}

void HiddenAreaMeshApp::Shader::Release()
{
    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_fs);
    SAFE_RELEASE(m_shd);
}

HiddenAreaMeshApp::HiddenAreaMeshApp()
{
    m_Cube = IZ_NULL;
    m_Img = IZ_NULL;

    izanagi::math::SMatrix44::SetUnit(m_L2W);
}

HiddenAreaMeshApp::~HiddenAreaMeshApp()
{
}

// 初期化.
IZ_BOOL HiddenAreaMeshApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    IZ_UINT flag = izanagi::E_DEBUG_MESH_VTX_FORM_POS;

    // Cube
    {
        m_Cube = izanagi::sample::CSampleEnvBox::CreateSampleEnvBox(
            allocator,
            device);
        VGOTO(result = (m_Cube != IZ_NULL), __EXIT__);
    }

    // テクスチャ
    {
        izanagi::CFileInputStream in;
        VGOTO(result = in.Open("data/EnvMap.img"), __EXIT__);

        m_Img = izanagi::CImage::CreateImage(
                    allocator,
                    device,
                    &in);
        VGOTO(result = (m_Img != IZ_NULL), __EXIT__);
    }

    // FBO.
    {
        m_rt = device->CreateRenderTarget(
            SCREEN_WIDTH, SCREEN_HEIGHT,
            izanagi::graph::E_GRAPH_PIXEL_FMT_RGBA8);

        m_depth = device->CreateRenderTarget(
            SCREEN_WIDTH, SCREEN_HEIGHT,
            izanagi::graph::E_GRAPH_PIXEL_FMT_D24S8);

        m_mask = device->CreateRenderTarget(
            SCREEN_WIDTH, SCREEN_HEIGHT,
            izanagi::graph::E_GRAPH_PIXEL_FMT_RGBA8);
    }

    m_shdDrawCube.Init(
        device,
        "shader/vs_equirect.glsl",
        "shader/fs_equirect.glsl");

    IZ_FLOAT aspect = (IZ_FLOAT)SCREEN_WIDTH / SCREEN_HEIGHT;
    IZ_FLOAT verticalFOV = izanagi::math::CMath::Deg2Rad(60.0f);

    // カメラ
    camera.Init(
        izanagi::math::CVector4(0.0f, 0.0f,  0.0f, 1.0f),
        izanagi::math::CVector4(0.0f, 0.0f,  1.0f, 1.0f),
        izanagi::math::CVector4(0.0f, 1.0f,  0.0f, 1.0f),
        1.0f,
        500.0f,
        verticalFOV,
        aspect);
    camera.Update();

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

// 解放.
void HiddenAreaMeshApp::ReleaseInternal()
{
    SAFE_RELEASE(m_Cube);
    SAFE_RELEASE(m_Img);

    m_shdDrawCube.Release();

    SAFE_RELEASE(m_mask);

    SAFE_RELEASE(m_rt);
    SAFE_RELEASE(m_depth);
}

// 更新.
void HiddenAreaMeshApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    GetCamera().Update();

    izanagi::math::SMatrix44::SetScale(m_L2W, 100.0f, 100.0f, 100.0f);

    // カメラの位置にあわせて移動する
    izanagi::math::SMatrix44::Trans(
        m_L2W,
        m_L2W,
        GetCamera().GetParam().pos);
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
void HiddenAreaMeshApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    izanagi::sample::CSampleCamera& camera = GetCamera();

    auto vp = device->GetViewport();

    device->SetViewport(
        izanagi::graph::SViewport(
        0, 0,
        SCREEN_WIDTH / 2, SCREEN_HEIGHT));
    {
        device->Begin2D();
        {
            device->SetRenderState(
                izanagi::graph::E_GRAPH_RS_STENCIL_ENABLE,
                IZ_TRUE);

            // 描画範囲にステンシル１を付加する.
            device->SetStencilFunc(
                izanagi::graph::E_GRAPH_CMP_FUNC_ALWAYS,
                1,
                0xffffffff);

            // これから描画するもののステンシル値にすべて１タグをつける.
            device->SetStencilOp(
                izanagi::graph::E_GRAPH_STENCIL_OP_REPLACE,
                izanagi::graph::E_GRAPH_STENCIL_OP_REPLACE,
                izanagi::graph::E_GRAPH_STENCIL_OP_REPLACE);

            // カラー・デプスバッファに書き込みしない.
            device->SetRenderState(
                izanagi::graph::E_GRAPH_RS_ZENABLE,
                IZ_FALSE);
            device->SetRenderState(
                izanagi::graph::E_GRAPH_RS_ZWRITEENABLE,
                IZ_FALSE);
            device->SetRenderState(
                izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_RGB,
                IZ_FALSE);
            device->SetRenderState(
                izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_A,
                IZ_FALSE);

            // Distortionのことを考えて、少し余裕を持たせる.
            IZ_INT Radius = SCREEN_HEIGHT / 2 * 1.05;

            IZ_INT CenterX = SCREEN_WIDTH / 4;
            IZ_INT CenterY = SCREEN_HEIGHT / 2;

            izanagi::CIntPoint p0(CenterX, CenterY);
            izanagi::CIntPoint p1;
            izanagi::CIntPoint p2(CenterX - Radius, CenterY);

            static const IZ_UINT Div = 20;
            for (IZ_UINT i = 0; i < Div; i++) {
                IZ_FLOAT theta = 360.0f / Div;
                theta *= (i + 1);

                auto t = IZ_DEG2RAD(theta);

                IZ_FLOAT x = cosf(t);
                IZ_FLOAT y = sinf(t);

                p1.x = CenterX - x * Radius;
                p1.y = CenterY + y * Radius;

                device->Draw2DTriangle(p0, p1, p2, 0xffffffff);

                p2 = p1;
            }
        }
        device->End2D();

        {
            device->SetRenderState(
                izanagi::graph::E_GRAPH_RS_STENCIL_ENABLE,
                IZ_TRUE);

            // 0と一致する場合にテストを通す.
            device->SetStencilFunc(
                izanagi::graph::E_GRAPH_CMP_FUNC_NOTEQUAL,
                0,
                0xffffffff);

            // ステンシルは書き換えない.
            device->SetStencilOp(
                izanagi::graph::E_GRAPH_STENCIL_OP_KEEP,
                izanagi::graph::E_GRAPH_STENCIL_OP_KEEP,
                izanagi::graph::E_GRAPH_STENCIL_OP_KEEP);
        }

        device->SetTexture(0, m_Img->GetTexture(m_Idx));

        auto shd = m_shdDrawCube.m_shd;

        device->SetShaderProgram(shd);

        auto hL2W = shd->GetHandleByName("g_mL2W");
        shd->SetMatrix(device, hL2W, m_L2W);

        auto mtxW2C = camera.GetParam().mtxW2C;

        auto hW2C = shd->GetHandleByName("g_mW2C");
        shd->SetMatrix(device, hW2C, mtxW2C);

        auto hEye = shd->GetHandleByName("g_vEye");
        shd->SetVector(device, hEye, camera.GetParam().pos);

        m_Cube->Render(device);
    }

    device->SetViewport(vp);
}

IZ_BOOL HiddenAreaMeshApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
