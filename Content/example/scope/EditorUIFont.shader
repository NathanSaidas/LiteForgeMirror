$property
cbuffer SHADER_PROPERTIES : register(b0)
{
    float4x4 uTransform;
    float4x4 uModel;
    float4   uColor;
    float4   uClipRect;
};
Texture2D<float> uFontMap    : register(t0);
SamplerState uFontMapSampler : register(s0);
$end

$common
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float2 objectPos : TEXCOORD1;
};
$end

$vertex
struct VSInput
{
    float4 position : SV_POSITION;
};

PSInput main(VSInput IN)
{
    PSInput OUT;
    OUT.position = mul(uTransform, float4(IN.position.xy, 0.0, 1.0));
    OUT.texCoord = float2(IN.position.zw);
    OUT.objectPos = mul(uModel, float4(IN.position.xy, 0.0, 1.0));
    return OUT;
}
$end

$pixel
#if defined(FXAA)
float FXAA_SCROLL = 0.1f;
#endif

float4 Sample2D(float2 texCoord, float2 offset)
{
    return uFontMap.Sample(uFontMapSampler, texCoord + float2(offset.x, offset.y));
}


float4 ClipAlpha(float2 position, float alpha)
{
    if (position.x > uClipRect.x && position.x < uClipRect.z &&
        position.y > uClipRect.y && position.y < uClipRect.w)
    {
        return alpha;
    }
    return 0.0;
}


float4 main(PSInput IN) : SV_TARGET
{
#if defined(FXAA)
    float4 north = Sample2D(IN.texCoord, float2(0.0, FXAA_SCROLL));
    float4 south = Sample2D(IN.texCoord, float2(0.0, -FXAA_SCROLL));
    float4 west = Sample2D(IN.texCoord, float2(-FXAA_SCROLL, 0.0));
    float4 east = Sample2D(IN.texCoord, float2(FXAA_SCROLL, 0.0));

    float3 finalColor = float3(0.0, 0.0,0.0);
    finalColor += north;
    finalColor += south;
    finalColor += west;
    finalColor += east;
    finalColor /= 4;
    float alpha = ClipAlpha(IN.objectPos.xy, min(finalColor.r, 1.0));
    return float4(uColor.r, uColor.g, uColor.b, alpha);
#else 
    float4 color = uFontMap.Sample(uFontMapSampler, IN.texCoord);
    return float4(uColor.r, uColor.g, uColor.b, color.r);
#endif
}
$end   