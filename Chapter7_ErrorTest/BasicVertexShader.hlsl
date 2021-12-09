#include "BasicShaderHeader.hlsli"

//�ϊ����܂Ƃ߂��\����
cbuffer cbuff0 : register(b0)
{
    matrix mat; //�ϊ��s��
};

Output BasicVS(
    float4 pos : POSITION,
    float4 normal : NORMAL,
    float2 uv : TEXCOORD,
    min16uint2 boneno : BONE_NO,
    min16uint weight : WEIGHT)
{
    Output output; //�n���l
    output.svpos = mul(mat, pos); //HLSL�͗�D��Ȃ̂ō�����|����
    output.uv = uv;
    return output;
}