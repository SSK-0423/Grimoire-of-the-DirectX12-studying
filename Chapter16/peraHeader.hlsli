Texture2D<float4> tex : register(t0);   //通常テクスチャ
Texture2D<float4> texNormal : register(t1); //法線テクスチャ
Texture2D<float4> texHighLum : register(t2);    //輝度テクスチャ
Texture2D<float4> texShrinkHighLum : register(t3);  //縮小バッファー高輝度
Texture2D<float4> texDof : register(t4);
Texture2D<float4> effectTex : register(t5); //エフェクト用テクスチャ
//深度値検証用
Texture2D<float> depthTex : register(t6);  //深度
Texture2D<float> lightDepthTex : register(t7); //ライトデプス
//アンビエントオクルージョン用
Texture2D<float> texSSAO : register(t8);    //SSAO

SamplerState smp : register(s0);

cbuffer PostEffect : register(b0)
{
    float4 bkweights[2];
};

//定数バッファ0
cbuffer SceneData : register(b1)
{
    matrix view;
    matrix proj; //ビュープロジェクション行列
    matrix invProj; //逆プロジェクション行列
    matrix lightCamera; //ライトビュープロジェクション
    matrix shadow; //影行列
    float3 eye;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct BlurOutput
{
    float4 highLum : SV_TARGET0; // 高輝度(High Luminance)
    float4 col : SV_TARGET1; // 通常のレンダリング結果
};