float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
    return float4(pos.xy * 0.5f, 0, 1);
}