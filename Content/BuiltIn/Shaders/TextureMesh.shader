SamplerState PointWrap          : register(s0);
SamplerState PointClamp         : register(s1);
SamplerState LinearWarp         : register(s2);
SamplerState LinearClamp        : register(s3);
SamplerState AnisotropicWrap    : register(s4);
SamplerState AnisotropicClamp   : register(s5);

Texture2D gTextures0 : register(t0);
Texture2D gTextures1 : register(t1);
Texture2D gTextures2 : register(t2);

struct PerObject
{
    float3 position;
};
ConstantBuffer<PerObject> gPerObject : register(b0);

StructuredBuffer<PerObject> gPerObjectStruct : register(t3);

struct PSInput
{
    float4 position : SV_POSITION;
    float4 tintColor : COLOR0;
    float2 texCoord : TEXCOORD0;
};

#if defined(LF_VERTEX)
struct VSInput
{
    float4 position : SV_POSITION;
    float4 tintColor : COLOR0;
    float2 texCoord : TEXCOORD0;
};

PSInput VSMain(VSInput IN)
{
    PSInput o;
    o.position = IN.position;
    o.tintColor = IN.tintColor;
    o.texCoord = IN.texCoord;
    return o;
}
#endif

#if defined(LF_PIXEL)

float4 PSMain(PSInput IN) : SV_TARGET
{
    float4 a = float4(gTextures2.Sample(PointWrap, IN.texCoord.xy).xyz, 1.0);
    // float4 b = float4(gPerObject.position, 1.0);
    float4 b = float4(gPerObjectStruct[0].position, 1.0);
    float4 c = a * b;

    // return float4(IN.texCoord.xy,0,1);
    return c;
}

#endif