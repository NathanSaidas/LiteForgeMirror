$property

// Some helpful defines: 
//   RENDER_RECT   - Renders as rect with rect outline. (Determined by tex coords) 
//   RENDER_CIRCLE - Renders as circle with circle outline. (Determined by window position)
//   RENDER_TEXTURE - Renders as a rect texture.

cbuffer SHADER_PROPERTIES : register(b0)
{
    float4x4 uProjection;
    float4x4 uModel;
    float4 uBackground; // Aka Outline Color
    float4 uForeground; // 
#if defined(RENDER_CIRCLE)
    float4 uCircleParams; // ( x,y = circle center | z = radius )
#elif defined(RENDER_RECT)
    float2 uOutline;      // ( x,y clamped [0-1] )
#endif
};
#if defined(RENDER_TEXTURE)
Texture2D uTexture : register(t0);
SamplerState uTextureSampler : register(s0);
#endif
$end

$common
struct PSInput
{
    float4 position : SV_POSITION;
    float4 model    : POSITION;
    float2 texCoord : TEXCOORD0;
};
$end

$vertex
struct VSInput
{
    float4 position : SV_POSITION; // (x,y = position | z,w = texCoord )
};

#define EditorUI \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT )," \
"DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_ALL ),"\
"CBV(b0, visibility = SHADER_VISIBILITY_ALL ), " \
"StaticSampler(s0,"\
"           filter =   FILTER_MIN_MAG_MIP_LINEAR,"\
"           addressU = TEXTURE_ADDRESS_CLAMP,"\
"           addressV = TEXTURE_ADDRESS_CLAMP,"\
"           addressW = TEXTURE_ADDRESS_CLAMP,"\
"           visibility = SHADER_VISIBILITY_ALL )"

[RootSignature(EditorUI)]
PSInput main(VSInput IN)
{
    float4x4 transform = mul(uProjection, uModel);
    PSInput OUT;
    OUT.position = mul(transform, float4(IN.position.xy, 0.0, 1.0));
    OUT.model = mul(uModel, float4(IN.position.xy, 0.0,1.0));
    OUT.texCoord = float2(IN.position.zw);
    return OUT;
}
$end

$pixel

#if defined(RENDER_CIRCLE)
// Checks if a point is within circle bound
// @params - Circle Params( xy = target point, z = radius )
// @pos - Window Space ( position of fragment )
// @return - 0 if outside of radius, 1 if inside radius
float GetCircleClip(float3 params, float2 pos)
{
    float dist = length(pos - params.xy);
    return step(dist, params.z);
}

// Checks if a point should be outlined.
// @param - Circle Params( xy = target point, z = radius ) 
// @pos - Window Space ( positoin of fragment )
// @return - 1 if outlined, 0 if not outlined
float GetCircleOutline(float4 params, float2 pos)
{
    float outline = params.w;
    float radius = params.z * (1.0 - outline);
    float dist = length(pos - params.xy);
    return step(radius, dist);
}

float GetCircleToggleOutline(float4 params, float2 pos)
{
    float outline = params.w;
    float outlineSqr = outline * outline;
    float innerRadius = params.z * (1.0 - outlineSqr);
    float outerRadius = params.z * (1.0 - outline);
    float dist = length(pos - params.xy);
    if (dist < outerRadius || dist > innerRadius)
    {
        return 1.0;
    }
    return 0.0;
}
#elif defined(RENDER_RECT) 
// Checks if a point is within bounds for outlining effect.
// @param x,y - texture coords
// @param px,py - border percent
float GetRectOutline(float x, float y, float px, float py)
{
    if (
        x < (px) || x >(1.0 - px) ||
        y < (py) || y >(1.0 - py)
        )
    {
        return 1.0;
    }
    return 0.0;
}
#elif defined(RENDER_TEXTURE)
float4 Sample2D(float2 texCoord, float2 offset)
{
    return uTexture.Sample(uTextureSampler, texCoord + float2(offset.x, offset.y));
}
#endif 

float4 main(PSInput IN) : SV_TARGET
{
#if defined(RENDER_CIRCLE)
    float4 finalColor = lerp(uForeground, uBackground, GetCircleOutline(uCircleParams, IN.model.xy));
    finalColor.a = GetCircleClip(uCircleParams, IN.model.xy);
#elif defined(RENDER_RECT)
    float mixer = GetRectOutline(IN.texCoord.x, IN.texCoord.y, uOutline.x, uOutline.y);
    float4 finalColor = lerp(uForeground, uBackground, mixer);
#elif defined(RENDER_TEXTURE)
    float4 finalColor = Sample2D(IN.texCoord, float2(0.0, 0.0));
#endif
    return finalColor;
}
$end