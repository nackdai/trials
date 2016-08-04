#include <vector>
#include "SingleImageStereoApp.h"

// NOTE
// http://research.lighttransport.com/foveated-real-time-ray-tracing-for-virtual-reality-headset/asset/abstract.pdf

const IZ_FLOAT SingleImageStereoApp::IPD = 6.0f;

SingleImageStereoApp::SingleImageStereoApp()
{
    m_Cube = IZ_NULL;
    m_Img = IZ_NULL;

    izanagi::math::SMatrix44::SetUnit(m_L2W);
}

SingleImageStereoApp::~SingleImageStereoApp()
{
}

// 初期化.
IZ_BOOL SingleImageStereoApp::InitInternal(
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

    // シェーダ
    {
        izanagi::CFileInputStream in;
        VGOTO(result = in.Open("shader/vs_equirect.glsl"), __EXIT__);

        std::vector<uint8_t> buf(in.GetSize() + 1);
        in.Read(&buf[0], 0, in.GetSize());

        m_vs = device->CreateVertexShader(&buf[0]);

        VGOTO(result = (m_vs != IZ_NULL), __EXIT__);
    }
    {
        izanagi::CFileInputStream in;
        VGOTO(result = in.Open("shader/fs_equirect.glsl"), __EXIT__);

        std::vector<uint8_t> buf(in.GetSize() + 1);
        in.Read(&buf[0], 0, in.GetSize());

        m_fs = device->CreatePixelShader(&buf[0]);

        VGOTO(result = (m_fs != IZ_NULL), __EXIT__);
    }
    {
        m_shd = device->CreateShaderProgram();
        m_shd->AttachVertexShader(m_vs);
        m_shd->AttachPixelShader(m_fs);
    }

    // FBO.
    {
        izanagi::HmdInfo info;
#if 0
        // 重複するピクセル数を計算.
        auto overlapRatio = info.InterpupillaryDistance / info.HScreenSize;
        IZ_UINT overlapPixels = overlapRatio * info.HResolution + 0.5f;

        // 重複しない部分のピクセル数を計算.
        IZ_UINT stereoMarginPixels = info.HResolution * 0.5f - overlapPixels + 0.5f;

        // 左右をカバーするスクリーンサイズを計算.
        m_StereoScreenWidth = stereoMarginPixels * 2 + overlapPixels;
#else
        // NOTE
        // http://www.animesh.me/2011/05/rendering-3d-anaglyph-in-opengl.html

        // a typical DK1 half-FOV is 46 degrees inwards, 53 degrees outwards
        // https://developer.oculusvr.com/forums/viewtopic.php?p=111699#p111699

        IZ_FLOAT inwards = IZ_DEG2RAD(46);
        IZ_FLOAT outwards = IZ_DEG2RAD(53);

        IZ_FLOAT a = info.EyeToScreenDistance * (tanf(inwards) + tanf(outwards));
        IZ_FLOAT x = (a - info.InterpupillaryDistance) / 2;

        IZ_FLOAT h = a;

        IZ_FLOAT ratio = h / info.HScreenSize;
        m_StereoScreenWidth = ratio * info.HResolution + 0.5f;
#endif

        m_rt = device->CreateRenderTarget(
            m_StereoScreenWidth, SCREEN_HEIGHT,
            izanagi::graph::E_GRAPH_PIXEL_FMT_RGBA8);

        m_depth = device->CreateRenderTarget(
            m_StereoScreenWidth, SCREEN_HEIGHT,
            izanagi::graph::E_GRAPH_PIXEL_FMT_D24S8);
    }

    IZ_FLOAT aspect = (IZ_FLOAT)SCREEN_WIDTH / SCREEN_HEIGHT;
    IZ_FLOAT verticalFOV = izanagi::math::CMath::Deg2Rad(90.0f);
    IZ_FLOAT screenDist = SCREEN_HEIGHT / (2.0f * tanf(verticalFOV * 0.5f));
    IZ_FLOAT horizontalFOV = 2.0f * atanf(m_StereoScreenWidth / (2.0f * screenDist));

    IZ_FLOAT zn = 0.01f;
    IZ_FLOAT zf = 5.0f;

    // カメラ
    camera.Init(
        izanagi::math::CVector4(0.0f, 0.0f,  0.0f, 1.0f),
        izanagi::math::CVector4(0.0f, 0.0f,  1.0f, 1.0f),
        izanagi::math::CVector4(0.0f, 1.0f,  0.0f, 1.0f),
        zn, zf,
        verticalFOV,
        aspect);
    camera.Update();

    IZ_FLOAT thetaH = IZ_DEG2RAD(109.0f);
    IZ_FLOAT degreeThetaH = 0.0f;
#if 0
    {
        izanagi::HmdInfo info;

        IZ_FLOAT r = m_StereoScreenWidth / info.HResolution;
        IZ_FLOAT h = r * info.HScreenSize;

        thetaH = 2.0f * atanf((h * 0.5f) / info.EyeToScreenDistance);
        degreeThetaH = IZ_RAD2DEG(thetaH);
    }
#endif
    IZ_FLOAT thetaV = IZ_DEG2RAD(90.0f);

    auto& mtxV2C = const_cast<izanagi::math::SMatrix44&>(camera.GetParam().mtxV2C);
    auto& mtxW2C = const_cast<izanagi::math::SMatrix44&>(camera.GetParam().mtxW2C);

    IZ_FLOAT r = zn * tanf(thetaH / 2.0f);
    IZ_FLOAT l = -r;

    IZ_FLOAT t = zn * tanf(thetaV / 2.0f);
    IZ_FLOAT b = -t;

    izanagi::CSceneGraphUtil::ComputePerspectiveOffCenter(
        mtxV2C,
        l, r, b, t,
        zn, zf);

    izanagi::math::SMatrix44::Mul(
        mtxW2C,
        camera.GetParam().mtxW2V,
        mtxV2C);

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

// 解放.
void SingleImageStereoApp::ReleaseInternal()
{
    SAFE_RELEASE(m_Cube);
    SAFE_RELEASE(m_Img);

    SAFE_RELEASE(m_shd);
    SAFE_RELEASE(m_vs);
    SAFE_RELEASE(m_fs);

    SAFE_RELEASE(m_rt);
    SAFE_RELEASE(m_depth);
}

// 更新.
void SingleImageStereoApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
    GetCamera().Update();

    izanagi::math::SMatrix44::SetScale(m_L2W, 1, 1, 1);

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
void SingleImageStereoApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    izanagi::sample::CSampleCamera& camera = GetCamera();

    static IZ_INT texIdx[] = {
        1,
        3,
        4,
    };

    static IZ_CHAR* name[] = {
        "Cube",
        "Latitude-Longitude",
        "Angular",
    };

    device->BeginScene(
        &m_rt, 1,
        m_depth,
        izanagi::graph::E_GRAPH_CLEAR_FLAG_ALL);
    {
        device->SetRenderState(
            izanagi::graph::E_GRAPH_RS_CULLMODE,
            izanagi::graph::E_GRAPH_CULL_NONE);

        device->SetTexture(0, m_Img->GetTexture(texIdx[m_Idx]));

        device->SetShaderProgram(m_shd);

        auto hL2W = m_shd->GetHandleByName("g_mL2W");
        m_shd->SetMatrix(device, hL2W, m_L2W);

        device->SetViewport(
            izanagi::graph::SViewport(
            0, 0,
            m_StereoScreenWidth, SCREEN_HEIGHT));

        auto mtxW2C = camera.GetParam().mtxW2C;

        auto hW2C = m_shd->GetHandleByName("g_mW2C");
        m_shd->SetMatrix(device, hW2C, mtxW2C);

        auto hEye = m_shd->GetHandleByName("g_vEye");
        m_shd->SetVector(device, hEye, camera.GetParam().pos);

        m_Cube->Render(device);
    }
    device->EndScene();

    device->SetViewport(
        izanagi::graph::SViewport(
        0, 0,
        SCREEN_WIDTH, SCREEN_HEIGHT));

    if (device->Begin2D()) {
        device->SetTexture(0, m_rt);

        // Left.
        device->Draw2DSpriteEx(
            izanagi::CIntRect(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT),
            izanagi::CIntRect(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT));

        // Right.
        device->Draw2DSpriteEx(
            izanagi::CIntRect(m_StereoScreenWidth - SCREEN_WIDTH / 2, 0.0f, SCREEN_WIDTH / 2, SCREEN_HEIGHT),
            izanagi::CIntRect(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT));

        izanagi::CDebugFont* debugFont = GetDebugFont();

        debugFont->Begin(device, 0, izanagi::CDebugFont::FONT_SIZE * 2);

        debugFont->DBPrint(
            device, 
            "%s\n",
            name[m_Idx]);

        debugFont->End();

        device->End2D();
    }
}

IZ_BOOL SingleImageStereoApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
