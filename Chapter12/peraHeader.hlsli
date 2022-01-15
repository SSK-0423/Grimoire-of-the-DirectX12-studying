Texture2D<float4> tex : register(t0);   //通常テクスチャ
SamplerState smp : register(s0);
Texture2D<float4> effectTex : register(t1); //エフェクト用テクスチャ

cbuffer PostEffect : register(b0)
{
    float4 bkweights[2];
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};