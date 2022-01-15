#include "peraHeader.hlsli"

float4 filetring(Output output, float3x3 filter)
{
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dx = 1.f / w; //1�s�N�Z��������̈ړ���
    float dy = 1.f / h; //1�s�N�Z��������̈ړ���
    float4 ret = float4(0, 0, 0, 0);

    ret += tex.Sample(smp, output.uv + float2(-2 * dx, -2 * dy)) * filter._m00; //����
    ret += tex.Sample(smp, output.uv + float2(0, -2 * dy)) * filter._m01; //��
    ret += tex.Sample(smp, output.uv + float2(2 * dx, -2 * dy)) * filter._m02; //�E��
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 0)) * filter._m10; //��
    ret += tex.Sample(smp, output.uv + float2(0, 0)) * filter._m11; //����
    ret += tex.Sample(smp, output.uv + float2(2 * dx, 0)) * filter._m12; //�E
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 2 * dy)) * filter._m20; //����
    ret += tex.Sample(smp, output.uv + float2(0, 2 * dy)) * filter._m21; //��
    ret += tex.Sample(smp, output.uv + float2(2 * dx, 2 * dy)) * filter._m22; //�E��
    return ret;
}

float4 simpleGaussianBlur(Output output)
{
    float filter[5][5] =
    {
        { 1, 4, 6, 4, 1 },
        { 4, 16, 24, 16, 4 },
        { 6, 24, 36, 24, 6 },
        { 4, 16, 24, 16, 4 },
        { 1, 4, 6, 4, 1 },
    };
    
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dx = 1.f / w; //1�s�N�Z��������̈ړ���
    float dy = 1.f / h; //1�s�N�Z��������̈ړ���
    float4 ret = float4(0, 0, 0, 0);
    
    //�ŏ�i
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 2 * dy)) * filter[0][0]; //��2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, 2 * dy)) * filter[0][1]; //��
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 2 * dy)) * filter[0][2]; //��
    ret += tex.Sample(smp, output.uv + float2(1 * dx, 2 * dy)) * filter[0][3]; //�E
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 2 * dy)) * filter[0][4]; //�E2
    //��i
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 1 * dy)) * filter[1][0]; //��2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, 1 * dy)) * filter[1][1]; //��
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 1 * dy)) * filter[1][2]; //��
    ret += tex.Sample(smp, output.uv + float2(1 * dx, 1 * dy)) * filter[1][3]; //�E
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 1 * dy)) * filter[1][4]; //�E2
    //���i
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 0 * dy)) * filter[2][0]; //��2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, 0 * dy)) * filter[2][1]; //��
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 0 * dy)) * filter[2][2]; //��
    ret += tex.Sample(smp, output.uv + float2(1 * dx, 0 * dy)) * filter[2][3]; //�E
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 0 * dy)) * filter[2][4]; //�E2
    //���i
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, -1 * dy)) * filter[3][0]; //��2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, -1 * dy)) * filter[3][1]; //��
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -1 * dy)) * filter[3][2]; //��
    ret += tex.Sample(smp, output.uv + float2(1 * dx, -1 * dy)) * filter[3][3]; //�E
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -1 * dy)) * filter[3][4]; //�E2
    //�ŉ��i
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, -2 * dy)) * filter[4][0]; //��2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, -2 * dy)) * filter[4][1]; //��
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -2 * dy)) * filter[4][2]; //��
    ret += tex.Sample(smp, output.uv + float2(1 * dx, -2 * dy)) * filter[4][3]; //�E
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -2 * dy)) * filter[4][4]; //�E2
    
    return ret / 256;
}

float4 GaussianBlurX(Output output)
{
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dx = 1.f / w; //1�s�N�Z��������̈ړ���
    
    float4 col = tex.Sample(smp, output.uv);
    float4 ret = float4(0, 0, 0, 0);
    ret += bkweights[0] * col;
    
    for (int i = 1; i < 8; i++)
    {
        ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, output.uv + float2(i * dx, 0));
        ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, output.uv + float2(-i * dx, 0));
    }
    return float4(ret.rgb, col.a);
}



float4 grayScale(float4 color)
{
    return dot(color.rgb, float3(0.299, 0.587, 0.144));
}

float4 reverseCol(float4 color)
{
    return float4(float3(1.f, 1.f, 1.f) - color.rgb, color.a);
}

float4 ps(Output input) : SV_TARGET
{
    //�������t�B���^�[
    float3x3 smoothing =
    {
        { 1 / 9, 1 / 9, 1 / 9 },
        { 1 / 9, 1 / 9, 1 / 9 },
        { 1 / 9, 1 / 9, 1 / 9 }
    };
    //�G���{�X���H
    float3x3 emboss =
    {
        { 2, 1, 0 },
        { 1, 1, -1 },
        { 0, -1, -2 }
    };
    //�V���[�v�l�X(�G�b�W�̋���)
    float3x3 shapness =
    {
        { 0, -1, 0 },
        { -1, 5, -1 },
        { 0, -1, 0 }
    };
    //�G�b�W
    float3x3 edge =
    {
        { 0, -1, 0 },
        { -1, 4, -1 },
        { 0, -1, 0 }
    };
    
    return GaussianBlurX(input);
    //return simpleGaussianBlur(input);
    
    
    //float4 col = tex.Sample(smp, input.uv);
    //float edgeCol = filetring(input, edge);
    //float grayEdgeCol = grayScale(edgeCol);
    //return step(0.2, pow(reverseCol(grayEdgeCol), 10.f));
    
    //return float4(ret.rgb - fmod(ret.rbg, 0.25f), col.a);
    //return float4(float3(0.7f,0.8f,0.7f) - ret.rgb - fmod(ret.rbg, 0.25f), ret.a);
    //return float4(Y, Y, Y, 1);
    //return float4(float3(1.f, 1.f, 1.f) - col.rgb, col.a);
    //return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);
    //return tex.Sample(smp, input.uv);
    //return float4(input.uv, 1, 1);
}