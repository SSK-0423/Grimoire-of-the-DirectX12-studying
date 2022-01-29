#include"BasicType.hlsli"

float4 BasicPS(BasicType input) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1)); //���̌������x�N�g��(���s����)
    float3 lightColor = float3(1, 1, 1); //���C�g�̃J���[(1,1,1�Ő^����)

	//�f�B�t���[�Y�v�Z
    float diffuseB = saturate(dot(-light, input.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//���̔��˃x�N�g��
    float3 refLight = normalize(reflect(light, input.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
	
	//�X�t�B�A�}�b�v�pUV
    float2 sphereMapUV = input.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	//�e�N�X�`���J���[
    float4 texColor = tex.Sample(smp, input.uv);
    //�V���h�E�}�b�v�����̐F
    float4 ret = saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor
		+ float4(specularB * specular.rgb, 1))
		+ float4(texColor.rgb * ambient * 0.5, 1);
	
	//
    float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
    
    float2 shadowUV = (posFromLightVP + float2(1, -1)) * float2(0.5, -0.5);

    //float depthFromLight = lightDepthTex.Sample(smp, shadowUV);
    float depthFromLight = lightDepthTex.SampleCmp(
		shadowSmp, //��r�T���v���[
		shadowUV,	//uv�l
		posFromLightVP.z - 0.005f);	//��r�Ώےl
	
    float shadowWeight = 1.f;
    shadowWeight = lerp(0.5f, 1.f, depthFromLight);	//0.0�ɂȂ�����0.5�ɂȂ�悤��
    //if (depthFromLight < posFromLightVP.z - 0.001f)
    //    shadowWeight = 0.5f;
	
    if (input.instNo == 1)
    {
        return float4(0, 0, 0, 1);
    }
	/*
		�ǂ����A���C�g���猩���Ƃ��̐[�x���擾�ł��ĂȂ����ۂ�
		peraShader��lightDepth��\������ƁA��������擾�ł��Ă���̂ŁA
		�`�掞�̃f�B�X�N���v�^����̐ݒ肪���������̂��������[�g�V�O�l�`�����H
		�G���[�͏o�ĂȂ��̂ŁA�o�C���h����Ă��Ȃ����Ƃ͂Ȃ�����
		�^�����ɕ\������遨�����l�ɂȂ��Ă���
		�[�x�l�̏����l��������Ƃ��̒l�����f���ꂽ�̂Ńf�B�X�N���v�^����̐ݒ�͂����Ă���
		�ŏI�����_�����O�Ő[�x�����������Ă���̂��܂������H�����������ĂȂ�
		�V���h�E�p�X(���C�g�f�v�X������)��1�p�X��(���ʃf�v�X������)��2�p�X��(1�p�X�ڂ���̃e�N�X�`���\��)
	*/
	
    return float4(ret.rgb * shadowWeight, ret.a);
    return float4(depthFromLight, depthFromLight, depthFromLight, 1);
    return saturate(toonDif //�P�x(�g�D�[��)
		* diffuse //�f�B�t���[�Y�F
		* texColor //�e�N�X�`���J���[
		* sph.Sample(smp, sphereMapUV)) //�X�t�B�A�}�b�v(��Z)
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor //�X�t�B�A�}�b�v(���Z)
		+ float4(specularB * specular.rgb, 1)) //�X�y�L�����[
		+ float4(texColor * ambient * 0.5, 1); //�A���r�G���g(���邭�Ȃ肷����̂�0.5�ɂ��Ă܂�)

    return float4(shadowWeight, shadowWeight, shadowWeight, 1);
}