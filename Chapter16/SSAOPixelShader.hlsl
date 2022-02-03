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
    float dp = depthtex.Sample(smp, input.uv);  //���݂̐[�x
    
    float w, h, miplevels;
    depthtex.GetDimensions(0, w, h, miplevels);
    float dx = 1.f / w;
    float dy = 1.f / h;
    
    //SSAO
    //2.���̍��W�𕜌�����
    float4 respos = mul(invProj, float4(input.uv * float2(2, -2) + float2(-1, 1), dp, 1));
    
    respos.xyz = respos.xyz / respos.w;
    
    float div = 0.f;
    float ao = 0.f;
    float3 norm = normalize((normaltex.Sample(smp, input.uv).xyz * 2) - 1);
    const int trycnt = 256;
    const float radius = 0.5f;
    
    // �[�x��1�ȉ�=�J�����͈͓̔�
    if(dp < 1.f)
    {
        //3.�T���v���񐔌J��Ԃ�
        for (int i = 0; i < trycnt; i++)
        {
            //3.1 �����_������(�傫���͌Œ�)�Ƀx�N�g�����΂�
            float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
            float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
            float rnd3 = random(float2(rnd2, rnd1)) * 2 - 1;
            float3 omega = normalize(float3(rnd1, rnd2, rnd3));
            omega = normalize(omega);
            
            // �����̌��ʖ@���̔��Α��Ɍ����Ă����甽�]����
            float dt = dot(norm, omega);
            float sgn = sign(dt);   //�����擾
            omega *= sign(dt);
            
            // ���ʂ̍��W���Ăюˉe�ϊ�����
            // 3.2 
            float4 rpos = mul(proj, float4(respos.xyz + omega * radius, 1));
            rpos.xyz /= rpos.w;
            dt *= sgn;  // ���̒l�ɂ���cos�Ƃ𓾂�
            div += dt;  // �Օ����l���Ȃ����ʂ����Z����
            
            // �v�Z���ʂ����݂̏ꏊ�̐[�x��艜�ɓ����Ă���Ȃ�
            // �Օ�����Ă���̂ŉ��Z
            ao += step(depthtex.Sample(smp, (rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f)), rpos.z) * dt;
        }
        ao /= div;
    }
    return 1.f - ao;
}