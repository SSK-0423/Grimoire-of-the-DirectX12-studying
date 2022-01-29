#include "peraHeader.hlsli"

float4 VerticalBokehPS(Output input) : SV_TARGET
{
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dy = 1.f / h; //1�s�N�Z��������̈ړ���
    
    float4 col = tex.Sample(smp, input.uv);
    float4 ret = float4(0, 0, 0, 0);
    ret += bkweights[0] * col;
    
    for (int i = 1; i < 8; i++)
    {
        ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, i * dy));
        ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, -i * dy));
    }
    
    float2 nmTex = effectTex.Sample(smp, input.uv).xy;
    nmTex = nmTex * 2.f - 1;
    
    float depth = pow(depthTex.Sample(smp, input.uv), 20);
    //return float4(depth, depth, depth, 1);
    return float4(tex.Sample(smp, input.uv));
    
    //nmTex�͈̔͂�-1�`1�����A��1���e�N�X�`��1����
    //�傫���ł���-1�`1�ł͘c�݂��傫�����邽��0.1����Z���Ă���
    //return tex.Sample(smp, input.uv + nmTex * 0.1f);
    //return float4(ret.rgb, col.a);
}