#include "peraHeader.hlsli"

float4 ps(Output input) : SV_TARGET
{
    float4 col = tex.Sample(smp, input.uv);
    float Y = dot(col.rgb, float3(0.299, 0.587, 0.144));
    
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dx = 1.f / w; //1�s�N�Z��������̈ړ���
    float dy = 1.f / h; //1�s�N�Z��������̈ړ���
    float4 ret = float4(0, 0, 0, 0);
    
    ret += tex.Sample(smp, input.uv + float2(-128 * dx, -1 * dy));    //����
    ret += tex.Sample(smp, input.uv + float2(0, -128 * dy));          //��
    ret += tex.Sample(smp, input.uv + float2(128 * dx, -1 * dy));     //�E��
    ret += tex.Sample(smp, input.uv + float2(-128 * dx, 0));          //��
    ret += tex.Sample(smp, input.uv + float2(0,0));                 //����
    ret += tex.Sample(smp, input.uv + float2(128 * dx, 0));           //�E
    ret += tex.Sample(smp, input.uv + float2(-128 * dx, 1 * dy));     //����
    ret += tex.Sample(smp, input.uv + float2(0, 128 * dy));           //��
    ret += tex.Sample(smp, input.uv + float2(128 * dx, 1 * dy));      //�E��
    
    ret /= 9.f;
    //return float4(float3(0.7f,0.8f,0.7f) - ret.rgb - fmod(ret.rbg, 0.25f), ret.a);
    return float4(ret.rgb - fmod(ret.rbg, 0.25f), col.a);
    
    //return float4(Y, Y, Y, 1);
    //return float4(float3(1.f, 1.f, 1.f) - col.rgb, col.a);
    //return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);
    //return tex.Sample(smp, input.uv);
    //return float4(input.uv, 1, 1);
}