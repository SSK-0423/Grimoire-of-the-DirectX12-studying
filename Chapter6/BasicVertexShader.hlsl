#include "BasicShaderHeader.hlsli"

//変換をまとめた構造体
cbuffer cbuff0 : register(b0)
{
    matrix mat; //変換行列
};

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output; //渡す値
    output.svpos = mul(mat, pos);//HLSLは列優先なので左から掛ける
    //output.svpos = pos; //HLSLは列優先なので左から掛ける
    output.uv = uv;
    return output;
}