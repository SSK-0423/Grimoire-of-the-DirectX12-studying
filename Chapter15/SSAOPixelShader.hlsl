#include "peraHeader.hlsli"

//SSAOの処理のためだけのシェーダー
Texture2D<float4> normaltex : register(t1);//1パス目の法線描画
Texture2D<float> depthtex : register(t6);   //１パス目の深度テクスチャ
//現在のUV値を元に乱数を返す
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}
//SSAO(乗算用の明度のみ情報を返せればよい)
float SsaoPs(Output input) : SV_TARGET
{
    return 1.f;
}