Texture2D<float4> tex : register(t0);   //�ʏ�e�N�X�`��
SamplerState smp : register(s0);
Texture2D<float4> effectTex : register(t1); //�G�t�F�N�g�p�e�N�X�`��
//�[�x�l���ؗp
Texture2D<float> depthTex : register(t2);  //�[�x
Texture2D<float> lightDepthTex : register(t3); //���C�g�f�v�X

cbuffer PostEffect : register(b0)
{
    float4 bkweights[2];
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};