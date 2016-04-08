#include "OcclusionQueryApp.h"

static const IZ_FLOAT POS_X = -50.0f;
static const IZ_FLOAT DISTANCE = 10.0f;
static const IZ_UINT ORDER = 20;

OcclusionQueryApp::OcclusionQueryApp()
{
    m_sphere = IZ_NULL;
    m_cube = IZ_NULL;

    m_Img = IZ_NULL;

    m_Shader = IZ_NULL;
}

OcclusionQueryApp::~OcclusionQueryApp()
{
}

// 初期化.
IZ_BOOL OcclusionQueryApp::InitInternal(
    izanagi::IMemoryAllocator* allocator,
    izanagi::graph::CGraphicsDevice* device,
    izanagi::sample::CSampleCamera& camera)
{
    IZ_BOOL result = IZ_TRUE;

    IZ_UINT flag = izanagi::E_DEBUG_MESH_VTX_FORM_POS
        | izanagi::E_DEBUG_MESH_VTX_FORM_COLOR;

    IZ_FLOAT radius = 5.0f;
    IZ_FLOAT size = radius * 2.0f;

    m_plane = izanagi::CDebugMeshRectangle::CreateDebugMeshRectangle(
        allocator,
        device,
        flag,
        IZ_COLOR_RGBA(0xff, 0x00, 0x00, 0xff),
        1, 1,
        100.0f, 100.0f);

    flag = izanagi::E_DEBUG_MESH_VTX_FORM_POS
        | izanagi::E_DEBUG_MESH_VTX_FORM_COLOR
        | izanagi::E_DEBUG_MESH_VTX_FORM_UV;

    m_sphere = izanagi::CDebugMeshSphere::CreateDebugMeshSphere(
        allocator,
        device,
        flag,
        IZ_COLOR_RGBA(0xff, 0xff, 0xff, 0xff),
        radius,
        10, 10);
    VGOTO(result = (m_sphere != IZ_NULL), __EXIT__);

    flag = izanagi::E_DEBUG_MESH_VTX_FORM_POS;

    m_cube = izanagi::CDebugMeshBox::CreateDebugMeshBox(
        allocator,
        device,
        flag,
        IZ_COLOR_RGBA(0xff, 0xff, 0xff, 0xff),
        size, size, size);
    VGOTO(result = (m_cube != IZ_NULL), __EXIT__);

    for (IZ_UINT i = 0; i < MeshNum; i++) {
        SAFE_REPLACE(m_meshes[i].sphere, m_sphere);
        SAFE_REPLACE(m_meshes[i].cube, m_cube);

        IZ_FLOAT x = POS_X + DISTANCE * (i % ORDER);
        IZ_FLOAT y = 0.0f;
        IZ_FLOAT z = -DISTANCE * (i / ORDER);

        izanagi::math::SMatrix44::GetTrans(
            m_meshes[i].mtxL2W,
            x, y, z);

        CALL_GL_API(::glGenQueries(1, &m_meshes[i].query));
    }

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

    // カメラ
    camera.Init(
        izanagi::math::CVector4(0.0f, 10.0f, 50.0f, 1.0f),
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

// 解放.
void OcclusionQueryApp::ReleaseInternal()
{
    for (IZ_UINT i = 0; i < MeshNum; i++) {
        SAFE_RELEASE(m_meshes[i].sphere);
        SAFE_RELEASE(m_meshes[i].cube);

        CALL_GL_API(::glDeleteQueries(1, &m_meshes[i].query));
    }

    SAFE_RELEASE(m_sphere);
    SAFE_RELEASE(m_cube);

    SAFE_RELEASE(m_plane);

    SAFE_RELEASE(m_Img);

    SAFE_RELEASE(m_Shader);
}

// NOTE
// GPU Gems.
// Chapter 29. Efficient Occlusion Culling.
// http://http.developer.nvidia.com/GPUGems/gpugems_ch29.html

// 更新.
void OcclusionQueryApp::UpdateInternal(izanagi::graph::CGraphicsDevice* device)
{
#if 1
    if (!m_isFirstFrame) {
        // 前のフレームの結果でクエリ問い合わせ.
        for (IZ_UINT i = 0; i < MeshNum; i++) {
            GLuint pixelCnt = 0;
            CALL_GL_API(::glGetQueryObjectuiv(m_meshes[i].query, GL_QUERY_RESULT, &pixelCnt));
            m_meshes[i].willDraw = (pixelCnt > 0);
        }
    }
#endif

    m_isFirstFrame = IZ_FALSE;

    GetCamera().Update();
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
void OcclusionQueryApp::RenderInternal(izanagi::graph::CGraphicsDevice* device)
{
    izanagi::sample::CSampleCamera& camera = GetCamera();

    device->SetTexture(0, m_Img->GetTexture(0));

    IZ_UINT drawCount = 0;

    // シェーダ内でE_GRAPH_RS_ZWRITEENABLEの更新を無効にする.
    m_Shader->EnableToUpdateRenderState(
        izanagi::graph::E_GRAPH_RS_ZWRITEENABLE,
        IZ_FALSE);

#if 1
    m_Shader->Begin(device, 0, IZ_FALSE);
    {
        if (m_Shader->BeginPass(2)) {
            _SetShaderParam(
                m_Shader,
                "g_mW2C",
                (void*)&camera.GetParam().mtxW2C,
                sizeof(izanagi::math::SMatrix44));

            izanagi::math::CMatrix44 mtxL2W;
            mtxL2W.RotByX(IZ_DEG2RAD(90.0f));
            mtxL2W.Trans(0.0f, 0.0f, 20.0f);

            _SetShaderParam(
                m_Shader,
                "g_mL2W",
                (void*)&mtxL2W,
                sizeof(izanagi::math::SMatrix44));

            m_Shader->CommitChanges(device);

            m_plane->Draw(device);

            m_Shader->EndPass();
        }

        for (IZ_UINT i = 0; i < MeshNum; i++) {
            CALL_GL_API(::glBeginQuery(
                GL_SAMPLES_PASSED,
                m_meshes[i].query));

            if (m_meshes[i].willDraw) {
                drawCount++;

                if (m_Shader->BeginPass(0)) {
                    // パラメータ設定
                    _SetShaderParam(
                        m_Shader,
                        "g_mW2C",
                        (void*)&camera.GetParam().mtxW2C,
                        sizeof(izanagi::math::SMatrix44));

#if 0
                    glColorMask(true, true, true, true);
                    glDepthMask(GL_TRUE);
#else
                    // 元に戻す.
                    device->SetRenderState(
                        izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_RGB,
                        IZ_TRUE);
                    device->SetRenderState(
                        izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_A,
                        IZ_TRUE);
                    device->SetRenderState(
                        izanagi::graph::E_GRAPH_RS_ZWRITEENABLE,
                        IZ_TRUE);
#endif

                    _SetShaderParam(
                        m_Shader,
                        "g_mL2W",
                        (void*)&m_meshes[i].mtxL2W,
                        sizeof(izanagi::math::SMatrix44));

                    m_Shader->CommitChanges(device);

                    m_meshes[i].sphere->Draw(device);

                    m_Shader->EndPass();
                }
            }
            else {
                if (m_Shader->BeginPass(1)) {
                    // パラメータ設定
                    _SetShaderParam(
                        m_Shader,
                        "g_mW2C",
                        (void*)&camera.GetParam().mtxW2C,
                        sizeof(izanagi::math::SMatrix44));

#if 0
                    glColorMask(false, false, false, false);
                    glDepthMask(GL_FALSE);
#else
                    // 元に戻す.
                    device->SetRenderState(
                        izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_RGB,
                        IZ_FALSE);
                    device->SetRenderState(
                        izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_A,
                        IZ_FALSE);
                    device->SetRenderState(
                        izanagi::graph::E_GRAPH_RS_ZWRITEENABLE,
                        IZ_FALSE);
#endif

                    _SetShaderParam(
                        m_Shader,
                        "g_mL2W",
                        (void*)&m_meshes[i].mtxL2W,
                        sizeof(izanagi::math::SMatrix44));

                    m_Shader->CommitChanges(device);

                    m_meshes[i].cube->Draw(device);

                    m_Shader->EndPass();
                }
            }

            CALL_GL_API(::glEndQuery(GL_SAMPLES_PASSED));
        }
    }
    m_Shader->End(device);
#else
    m_Shader->Begin(device, 0, IZ_FALSE);

    glColorMask(false, false, false, false);
    glDepthMask(GL_FALSE);

    for (IZ_UINT i = 0; i < MeshNum; i++) {
        CALL_GL_API(::glBeginQuery(
            GL_SAMPLES_PASSED,
            m_meshes[i].query));

        if (m_Shader->BeginPass(1)) {
            // パラメータ設定
            _SetShaderParam(
                m_Shader,
                "g_mW2C",
                (void*)&camera.GetParam().mtxW2C,
                sizeof(izanagi::math::SMatrix44));

            _SetShaderParam(
                m_Shader,
                "g_mL2W",
                (void*)&m_meshes[i].mtxL2W,
                sizeof(izanagi::math::SMatrix44));

            m_Shader->CommitChanges(device);

            m_meshes[i].cube->Draw(device);

            m_Shader->EndPass();
        }

        CALL_GL_API(::glEndQuery(GL_SAMPLES_PASSED));

        int iSamplesPassed = 0;
        glGetQueryObjectiv(m_meshes[i].query, GL_QUERY_RESULT, &iSamplesPassed);
        m_meshes[i].willDraw = (iSamplesPassed > 0);
    }

    glColorMask(true, true, true, true);
    glDepthMask(GL_TRUE);

    for (IZ_UINT i = 0; i < MeshNum; i++) {
        if (m_meshes[i].willDraw) {
            drawCount++;

            if (m_Shader->BeginPass(0)) {
                // パラメータ設定
                _SetShaderParam(
                    m_Shader,
                    "g_mW2C",
                    (void*)&camera.GetParam().mtxW2C,
                    sizeof(izanagi::math::SMatrix44));

                _SetShaderParam(
                    m_Shader,
                    "g_mL2W",
                    (void*)&m_meshes[i].mtxL2W,
                    sizeof(izanagi::math::SMatrix44));

                m_Shader->CommitChanges(device);

                m_meshes[i].sphere->Draw(device);

                m_Shader->EndPass();
            }
        }
    }

    m_Shader->End(device);
#endif

    // 元に戻す.
    device->SetRenderState(
        izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_RGB,
        IZ_TRUE);
    device->SetRenderState(
        izanagi::graph::E_GRAPH_RS_COLORWRITEENABLE_A,
        IZ_TRUE);
    device->SetRenderState(
        izanagi::graph::E_GRAPH_RS_ZWRITEENABLE,
        IZ_TRUE);

    if (device->Begin2D()) {
        auto font = GetDebugFont();
        font->Begin(device, 0, 40);
        font->DBPrint(device, "Count[%d]\n", drawCount);
        font->End();
        device->End2D();
    }
}

IZ_BOOL OcclusionQueryApp::OnKeyDown(izanagi::sys::E_KEYBOARD_BUTTON key)
{
    return IZ_TRUE;
}
