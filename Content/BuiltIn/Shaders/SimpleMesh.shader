
struct PSInput
{
    float4 position : SV_POSITION;
};

#if defined(LF_VERTEX)
struct VSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(VSInput IN)
{
    PSInput o;
    o.position = IN.position;
    return o;
}
#endif

#if defined(LF_PIXEL)

float4 PSMain(PSInput IN) : SV_TARGET
{
    return float4(1,1,1,1);
}

#endif