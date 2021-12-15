#include"BasicType.hlsli"
Texture2D<float4> tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
SamplerState smp : register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��
SamplerState smpToon : register(s1); //1�ԃX���b�g�ɐݒ肳�ꂽ�T���v���[(�g�D�[���p)
Texture2D<float4> sph : register(t1); //1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��
Texture2D<float4> spa : register(t2); //2�ԃX���b�g��
Texture2D<float4> toon : register(t3);//3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�g�D�[��)

//�萔�o�b�t�@
cbuffer cbuff0 : register(b0)
{
    matrix world; //���[���h�ϊ��s��
    matrix view; //�r���[�s��
    matrix proj; //�v���W�F�N�V�����s��
    float3 eye; //���_
};

cbuffer Material : register(b1)
{
    float4 diffuse; //(XMFLOAT3 + float)
    float4 specular; //(XMFLOAT3 + float)
    float3 ambient;
};

float4 BasicPS(BasicType input) : SV_TARGET
{
    //���̌������x�N�g���i���s�����j
    float3 light = normalize(float3(1, -1, 1));
    
    //���C�g�̃J���[�i1,1,1�Ő^�����j
    float3 lightColor = float3(1, 1, 1);
    
    //�f�B�t���[�Y�v�Z
    float diffuseB = saturate(dot(-light, input.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));
    
    //���̔��˃x�N�g��
    float3 refLight = normalize(reflect(light, input.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
    
    //�X�t�B�A�}�b�v�pUV
    float2 sphereMapUV = input.vnormal.xy; //�X�t�B�A�}�b�v�p�@��
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);
    
    //�e�N�X�`���J���[
    float4 texColor = tex.Sample(smp, input.uv); //�e�N�X�`���J���[

    return max(saturate(toonDif //�P�x
            * diffuse //�f�B�t���[�Y�F
            * texColor //�e�N�X�`���J���[
            * sph.Sample(smp, sphereMapUV)) //�X�t�B�A�}�b�v(��Z)
            + spa.Sample(smp, sphereMapUV) * texColor //�X�t�B�A�}�b�v(���Z)
            + float4(specularB * specular.rgb, 1) //�X�y�L����
            , float4(texColor * ambient, 1)); //�A���r�G���g
    
    //return float4(brightness, brightness, brightness, 1) //�P�x
    //    * diffuse //�f�B�t���[�Y�F
    //    * color
    //    * tex.Sample(smp, input.uv) // �e�N�X�`���J���[
    //    * sph.Sample(smp, sphereMapUV) // �X�t�B�A�}�b�v(��Z)
    //    + spa.Sample(smp, sphereMapUV) // �X�t�B�A�}�b�v(���Z)
    //    + float4(color * ambient, 1);

    //return float4(brightness, brightness, brightness, 1) //�P�x
    //    * diffuse //�f�B�t���[�Y�F
    //    * tex.Sample(smp, input.uv) // �e�N�X�`���J���[
    //    * sph.Sample(smp, normalUV) // �X�t�B�A�}�b�v(��Z)
    //    + spa.Sample(smp, normalUV); // �X�t�B�A�}�b�v(���Z)
    

    //return float4(specularB * specular.rgb, 1);
	//return float4(input.normal.xyz,1);
	//return float4(tex.Sample(smp,input.uv));
}