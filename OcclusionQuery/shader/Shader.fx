// Basic Shader

struct SVSInput {
    float4 vPos     : POSITION;
    float4 vColor   : COLOR;
    float2 vUV      : TEXCOORD0;
};

struct SPSInput {
    float4 vPos     : POSITION;
    float4 vColor   : COLOR;
    float2 vUV      : TEXCOORD0;
};

#define SVSOutput        SPSInput

/////////////////////////////////////////////////////////////

float4x4 g_mL2W;
float4x4 g_mW2C;

texture tex;

sampler sTex = sampler_state
{
    Texture = tex;
};

/////////////////////////////////////////////////////////////

SVSOutput mainVS(SVSInput In)
{
    SVSOutput Out = (SVSOutput)0;

    Out.vPos = mul(In.vPos, g_mL2W);
    Out.vPos = mul(Out.vPos, g_mW2C);

    Out.vUV = In.vUV;
    Out.vColor = In.vColor;

    return Out;
}

float4 mainPS(SPSInput In) : COLOR
{
    float4 vOut = tex2D(sTex, In.vUV);

    vOut.rgb *= In.vColor.rgb;
    vOut.rgb *= In.vColor.a;

    return vOut;
}

float4 mainPS_NoTex(SPSInput In) : COLOR
{
    float4 vOut = In.vColor;
    return vOut;
}

/////////////////////////////////////////////////////////////

struct SVSInputQuery {
    float4 vPos     : POSITION;
};

struct SPSInputQuery {
    float4 vPos     : POSITION;
};

#define SVSOutputQuery        SPSInputQuery

SVSOutput mainVSQuery(SVSInput In)
{
    SVSOutput Out = (SVSOutput)0;

    Out.vPos = mul(In.vPos, g_mL2W);
    Out.vPos = mul(Out.vPos, g_mW2C);

    return Out;
}

float4 mainPSQuery(SPSInput In) : COLOR
{
    float4 vOut = 1.0f;

    return vOut;
}

/////////////////////////////////////////////////////////////

technique BasicShader
{
    pass basic
    {
        VertexShader = compile vs_2_0 mainVS();
        PixelShader = compile ps_2_0 mainPS();
    }
    pass query
    {
        VertexShader = compile vs_2_0 mainVSQuery();
        PixelShader = compile ps_2_0 mainPSQuery();
    }
    pass notex
    {
        VertexShader = compile vs_2_0 mainVS();
        PixelShader = compile ps_2_0 mainPS_NoTex();
    }
}
