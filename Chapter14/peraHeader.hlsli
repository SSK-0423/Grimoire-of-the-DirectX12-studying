Texture2D<float4> tex : register(t0);   //通常テクスチャ
Texture2D<float4> texNormal : register(t1); //法線テクスチャ
Texture2D<float4> texHighLum : register(t2);    //輝度テクスチャ
SamplerState smp : register(s0);
Texture2D<float4> effectTex : register(t3); //エフェクト用テクスチャ
//深度値検証用
Texture2D<float> depthTex : register(t4);  //深度
Texture2D<float> lightDepthTex : register(t5); //ライトデプス

cbuffer PostEffect : register(b0)
{
    float4 bkweights[2];
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};