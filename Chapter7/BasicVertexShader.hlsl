#include "BasicShaderHeader.hlsli"

Texture2D<float4> tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

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
    //output.svpos = mul(, pos);
    //output.svpos = pos;
    output.uv = uv;
    return output;
}