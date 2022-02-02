Texture2D<float4> tex : register(t0);   //�ʏ�e�N�X�`��
Texture2D<float4> texNormal : register(t1); //�@���e�N�X�`��
Texture2D<float4> texHighLum : register(t2);    //�P�x�e�N�X�`��
Texture2D<float4> texShrinkHighLum : register(t3);  //�k���o�b�t�@�[���P�x
Texture2D<float4> texDof : register(t4);
Texture2D<float4> effectTex : register(t5); //�G�t�F�N�g�p�e�N�X�`��
//�[�x�l���ؗp
Texture2D<float> depthTex : register(t6);  //�[�x
Texture2D<float> lightDepthTex : register(t7); //���C�g�f�v�X
//�A���r�G���g�I�N���[�W�����p
Texture2D<float> texSSAO : register(t8);    //SSAO

SamplerState smp : register(s0);

cbuffer PostEffect : register(b0)
{
    float4 bkweights[2];
};

//�萔�o�b�t�@0
cbuffer SceneData : register(b1)
{
    matrix view;
    matrix proj; //�r���[�v���W�F�N�V�����s��
    matrix invProj; //�t�v���W�F�N�V�����s��
    matrix lightCamera; //���C�g�r���[�v���W�F�N�V����
    matrix shadow; //�e�s��
    float3 eye;
};

struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct BlurOutput
{
    float4 highLum : SV_TARGET0; // ���P�x(High Luminance)
    float4 col : SV_TARGET1; // �ʏ�̃����_�����O����
};