#include "BasicType.hlsli"

//�e�p���_���W�ϊ�(���W�ϊ��̂�)
//���C�g�J�������_
//�֋X��APMD�Ɠ��l�̈����ɂ��Ă���
float4 shadowVS(
	float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT) : SV_POSITION
{
    float fWeight = float(weight) / 100.f;
	
    matrix conBone = bones[boneno.x] * fWeight + bones[boneno.y] * (1.f - fWeight);
	
    pos = mul(world, mul(conBone, pos));
	
    return mul(lightCamera, pos);
}