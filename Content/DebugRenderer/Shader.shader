$property
cbuffer SHADER_PROPERTIES : register(b0)
{
    float4x4 uTransform;
    float4   uColor;
}
$end

$common
struct PSInput
{
    float4 position : SV_POSITION;
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
    OUT.position = mul(uTransform, float4(IN.position.xyz, 1.0));
    return OUT;
}
$end

$pixel
float4 main(PSInput IN) : SV_TARGET
{
    return float4(uColor);
}
$end
