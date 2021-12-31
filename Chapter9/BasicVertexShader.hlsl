#include"BasicType.hlsli"
Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); //0番スロットに設定されたサンプラ
//定数バッファ
cbuffer cbuff0 : register(b0)
{
    matrix world; //ワールド変換行列
    matrix view; //ビュー行列
    matrix proj; //プロジェクション行列
    float3 eye;  //視点
};


BasicType BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONE_NO, min16uint weight : WEIGHT)
{
    BasicType output; //ピクセルシェーダへ渡す値
    pos = mul(world, pos);
    output.svpos = mul(mul(proj, view), pos); //シェーダでは列優先なので注意
    output.pos = mul(view, pos);
    normal.w = 0; //ここ重要(平行移動成分を無効にする)
    output.normal = mul(world, normal); //法線にもワールド変換を行う
    output.vnormal = mul(view, output.normal); //法線回転
    output.uv = uv;
    output.ray = normalize(pos.xyz - eye);
    return output;
}
