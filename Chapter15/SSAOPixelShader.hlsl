#include "peraHeader.hlsli"

//SSAO�̏����̂��߂����̃V�F�[�_�[
Texture2D<float4> normaltex : register(t1);//1�p�X�ڂ̖@���`��
Texture2D<float> depthtex : register(t6);   //�P�p�X�ڂ̐[�x�e�N�X�`��
//���݂�UV�l�����ɗ�����Ԃ�
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}
//SSAO(��Z�p�̖��x�̂ݏ���Ԃ���΂悢)
float SsaoPs(Output input) : SV_TARGET
{
    return 1.f;
}