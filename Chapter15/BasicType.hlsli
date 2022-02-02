Texture2D<float4> tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�x�[�X)
Texture2D<float4> sph : register(t1); //1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(��Z)
Texture2D<float4> spa : register(t2); //2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(���Z)
Texture2D<float4> toon : register(t3); //3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�g�D�[��)
//�V���h�E�}�b�v�p���C�h�[�x�e�N�X�`��
Texture2D<float> lightDepthTex : register(t4);

SamplerState smp : register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��
SamplerState smpToon : register(s1); //1�ԃX���b�g�ɐݒ肳�ꂽ�T���v��
SamplerComparisonState shadowSmp : register(s2);

//���_�V�F�[�_���s�N�Z���V�F�[�_�ւ̂����Ɏg�p����
//�\����
struct BasicType {
	float4 svpos:SV_POSITION;//�V�X�e���p���_���W
	float4 pos:POSITION;//���_���W
	float4 normal:NORMAL0;//�@���x�N�g��
	float4 vnormal:NORMAL1;//�@���x�N�g��
	float2 uv:TEXCOORD;//UV�l
	float3 ray:VECTOR;//�x�N�g��
    uint instNo : SV_InstanceID;
    float4 tpos : TPOS;
};

//�萔�o�b�t�@0
cbuffer SceneData : register(b0)
{
    matrix view;
    matrix proj; //�r���[�v���W�F�N�V�����s��
    matrix invPro;  //�t�v���W�F�N�V�����s��
    matrix lightCamera; //���C�g�r���[�v���W�F�N�V����
    matrix shadow; //�e�s��
    float3 eye;
};
cbuffer Transform : register(b1)
{
    matrix world; //���[���h�ϊ��s��
    matrix bones[256]; //�{�[���s�� ���I�m�ۂ͂ł��Ȃ����߂��炩���ߕK�v�ɂȂ肻���Ȑ������m�ۂ��Ă���
}

//�萔�o�b�t�@1
//�}�e���A���p
cbuffer Material : register(b2)
{
    float4 diffuse; //�f�B�t���[�Y�F
    float4 specular; //�X�y�L����
    float3 ambient; //�A���r�G���g
};

struct PixelOutput
{
    float4 col : SV_TARGET0;    //�J���[�l���o��
    float4 normal : SV_TARGET1; //�@�����o��
	float4 highLum : SV_TARGET2;//���P�x(High Luminance)
};