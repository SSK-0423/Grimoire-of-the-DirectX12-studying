#include "BasicShaderHeader.hlsli"

//•ÏŠ·‚ğ‚Ü‚Æ‚ß‚½\‘¢‘Ì
cbuffer cbuff0 : register(b0)
{
    matrix mat; //•ÏŠ·s—ñ
};

Output BasicVS(
    float4 pos : POSITION,
    float4 normal : NORMAL,
    float2 uv : TEXCOORD,
    min16uint2 boneno : BONE_NO,
    min16uint weight : WEIGHT)
{
    Output output; //“n‚·’l
    output.svpos = mul(mat, pos); //HLSL‚Í—ñ—Dæ‚È‚Ì‚Å¶‚©‚çŠ|‚¯‚é
    output.uv = uv;
    return output;
}