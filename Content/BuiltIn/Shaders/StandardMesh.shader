
struct PSInput
{
    float4 position : SV_POSITION;
    float4 tintColor : COLOR0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

#if defined(LF_VERTEX)
struct VSInput
{
    float4 position : SV_POSITION;
    float4 tintColor : COLOR0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

PSInput VSMain(VSInput IN)
{
    PSInput o;
    o.position = IN.position;
    o.tintColor = IN.tintColor;
    o.texCoord = IN.texCoord;
    o.normal = IN.normal;
    return o;
}
#endif

#if defined(LF_PIXEL)

float4 PSMain(PSInput IN) : SV_TARGET
{
    return float4(IN.texCoord.xy,0,1);
}

#endif