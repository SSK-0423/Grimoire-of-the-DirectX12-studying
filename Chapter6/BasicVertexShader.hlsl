#include "BasicShaderHeader.hlsli"

//�ϊ����܂Ƃ߂��\����
cbuffer cbuff0 : register(b0)
{
    matrix mat; //�ϊ��s��
};

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output; //�n���l
    output.svpos = mul(mat, pos);//HLSL�͗�D��Ȃ̂ō�����|����
    //output.svpos = pos; //HLSL�͗�D��Ȃ̂ō�����|����
    output.uv = uv;
    return output;
}