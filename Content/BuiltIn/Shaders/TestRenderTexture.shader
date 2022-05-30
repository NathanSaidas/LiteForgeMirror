SamplerState PointWrap          : register(s0);
SamplerState PointClamp         : register(s1);
SamplerState LinearWarp         : register(s2);
SamplerState LinearClamp        : register(s3);
SamplerState AnisotropicWrap    : register(s4);
SamplerState AnisotropicClamp   : register(s5);

Texture2D gTextures0 : register(t0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

#if defined(LF_VERTEX)
struct VSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

PSInput VSMain(VSInput IN)
{
    PSInput o;
    o.position = IN.position;
    o.texCoord = IN.texCoord;
    return o;
}
#endif

#if defined(LF_PIXEL)

float4 PSMain(PSInput IN) : SV_TARGET
{
    return float4(gTextures0.Sample(PointClamp, IN.texCoord.xy).xyz, 1.0);
}

#endif