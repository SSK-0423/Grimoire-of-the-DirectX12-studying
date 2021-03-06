#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); //0番スロットに設定されたサンプラ

//変換をまとめた構造体
cbuffer cbuff0 : register(b0)
{
    matrix mat; //変換行列
};

Output BasicVS(
    float4 pos : POSITION,
    float4 normal : NORMAL,
    float2 uv : TEXCOORD,
    min16uint2 boneno : BONE_NO,
    min16uint weight : WEIGHT)
{
    Output output; //渡す値
    output.svpos = mul(mat, pos); //HLSLは列優先なので左から掛ける
    //output.svpos = mul(, pos);
    //output.svpos = pos;
    output.uv = uv;
    return output;
}