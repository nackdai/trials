#include "DXTCompressApp.h"

static const IZ_FLOAT POS_X = 0.0f;
static const IZ_FLOAT RADIUS = 5.0f;
static const IZ_FLOAT DISTANCE = RADIUS * 2.0f;
static const IZ_UINT ORDER = 20;
static const IZ_UINT THREAD_NUM = 6;

DXTCompressApp::DXTCompressApp()
{
    m_Mesh = IZ_NULL;

    m_Img = IZ_NULL;

    m_Shader = IZ_NULL;
}

DXTCompressApp::~DXTCompressApp()
{
}

// 初期化.
IZ_BOOL DXTCompressApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    IZ_UINT flag = izanagi::E_DEBUG_MESH_VTX_FORM_POS
                    | izanagi::E_DEBUG_MESH_VTX_FORM_COLOR
                    | izanagi::E_DEBUG_MESH_VTX_FORM_UV;

    m_Mesh = izanagi::CDebugMeshSphere::CreateDebugMeshSphere(
        allocator,
        device,
        flag,
        IZ_COLOR_RGBA(0xff, 0xff, 0xff, 0xff),
        RADIUS,
        100, 100);
    VGOTO(result = (m_Mesh != IZ_NULL), __EXIT__);

    // テクスチャ
    {
        izanagi::CFileInputStream in;
        VGOTO(result = in.Open("data/earth.img"), __EXIT__);

        m_Img = izanagi::CImage::CreateImage(
                    allocator,
                    device,
                    &in);
        VGOTO(result = (m_Img != IZ_NULL), __EXIT__);
    }

    // シェーダ
    {
        izanagi::CFileInputStream in;
        VGOTO(result = in.Open("data/Shader.shd"), __EXIT__);

        m_Shader = izanagi::shader::CShaderBasic::CreateShader<izanagi::shader::CShaderBasic>(
            allocator,
            device,
            &in);
        VGOTO(result = (m_Shader != IZ_NULL), __EXIT__);
    }

    for (IZ_UINT i = 0; i < MAX_MESH_NUM; i++) {
        izanagi::math::SMatrix44::GetTrans(
            m_objects[i].mtxL2W,
            POS_X + DISTANCE * (i % ORDER),
            0.0f,
            -DISTANCE * (i / ORDER));

        SAFE_REPLACE(m_objects[i].mesh, m_Mesh);
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

    m_capture.init(
        allocator,
        device,
        SCREEN_WIDTH, SCREEN_HEIGHT);

    m_dxtEncoder.init(
        allocator,
        device,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        "shader/2DVS.glsl",
        "shader/dxt.glsl",
        nullptr);

__EXIT__:
    if (!result) {
        ReleaseInternal();
    }

    return IZ_TRUE;
}

// 解放.
void DXTCompressApp::ReleaseInternal()
{
    SAFE_RELEASE(m_Mesh);

    SAFE_RELEASE(m_Img);

    SAFE_RELEASE(m_Shader);

    for (IZ_UINT i = 0; i < MAX_MESH_NUM; i++) {
        SAFE_RELEASE(m_objects[i].mesh);
    }

    m_capture.terminate();
    m_dxtEncoder.terminate();
}

// 更新.
void DXTCompressApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
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
void DXTCompressApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    izanagi::sample::CSampleCamera& camera = GetCamera();

    m_capture.begin(device);

    device->SetTexture(0, m_Img->GetTexture(0));

    m_Shader->Begin(device, 0, IZ_FALSE);
    {
        if (m_Shader->BeginPass(0)) {
            // パラメータ設定
            _SetShaderParam(
                m_Shader,
                "g_mW2C",
                (void*)&camera.GetParam().mtxW2C,
                sizeof(izanagi::math::SMatrix44));

            for (IZ_UINT i = 0; i < MAX_MESH_NUM; i++) {
                _SetShaderParam(
                    m_Shader,
                    "g_mL2W",
                    (void*)&m_objects[i].mtxL2W,
                    sizeof(izanagi::math::SMatrix44));

                m_Shader->CommitChanges(device);

                m_objects[i].mesh->Draw(device);
            }

            m_Shader->EndPass();
        }
    }
    m_Shader->End(device);

    m_capture.end(device);

    m_dxtEncoder.encode(
        device,
        m_capture.getColor());

    izanagi::CDebugFont* debugFont = GetDebugFont();

    if (device->Begin2D()) {
        m_capture.drawDebug(device);
        m_dxtEncoder.drawDebug(device);
        device->End2D();
    }
}

IZ_BOOL DXTCompressApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
