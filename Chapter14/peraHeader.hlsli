Texture2D<float4> tex : register(t0);   //通常テクスチャ
SamplerState smp : register(s0);
Texture2D<float4> effectTex : register(t1); //エフェクト用テクスチャ
//深度値検証用
Texture2D<float> depthTex : register(t2);  //深度
Texture2D<float> lightDepthTex : register(t3); //ライトデプス

cbuffer PostEffect : register(b0)
{
    float4 bkweights[2];
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};