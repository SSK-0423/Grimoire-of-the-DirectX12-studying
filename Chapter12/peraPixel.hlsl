#include "peraHeader.hlsli"

float4 filetring(Output output, float3x3 filter)
{
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dx = 1.f / w; //1ピクセル当たりの移動幅
    float dy = 1.f / h; //1ピクセル当たりの移動幅
    float4 ret = float4(0, 0, 0, 0);

    ret += tex.Sample(smp, output.uv + float2(-2 * dx, -2 * dy)) * filter._m00; //左上
    ret += tex.Sample(smp, output.uv + float2(0, -2 * dy)) * filter._m01; //上
    ret += tex.Sample(smp, output.uv + float2(2 * dx, -2 * dy)) * filter._m02; //右上
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 0)) * filter._m10; //左
    ret += tex.Sample(smp, output.uv + float2(0, 0)) * filter._m11; //自分
    ret += tex.Sample(smp, output.uv + float2(2 * dx, 0)) * filter._m12; //右
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 2 * dy)) * filter._m20; //左下
    ret += tex.Sample(smp, output.uv + float2(0, 2 * dy)) * filter._m21; //下
    ret += tex.Sample(smp, output.uv + float2(2 * dx, 2 * dy)) * filter._m22; //右下
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
    float dx = 1.f / w; //1ピクセル当たりの移動幅
    float dy = 1.f / h; //1ピクセル当たりの移動幅
    float4 ret = float4(0, 0, 0, 0);
    
    //最上段
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 2 * dy)) * filter[0][0]; //左2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, 2 * dy)) * filter[0][1]; //左
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 2 * dy)) * filter[0][2]; //中
    ret += tex.Sample(smp, output.uv + float2(1 * dx, 2 * dy)) * filter[0][3]; //右
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 2 * dy)) * filter[0][4]; //右2
    //上段
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 1 * dy)) * filter[1][0]; //左2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, 1 * dy)) * filter[1][1]; //左
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 1 * dy)) * filter[1][2]; //中
    ret += tex.Sample(smp, output.uv + float2(1 * dx, 1 * dy)) * filter[1][3]; //右
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 1 * dy)) * filter[1][4]; //右2
    //中段
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, 0 * dy)) * filter[2][0]; //左2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, 0 * dy)) * filter[2][1]; //左
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 0 * dy)) * filter[2][2]; //中
    ret += tex.Sample(smp, output.uv + float2(1 * dx, 0 * dy)) * filter[2][3]; //右
    ret += tex.Sample(smp, output.uv + float2(0 * dx, 0 * dy)) * filter[2][4]; //右2
    //下段
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, -1 * dy)) * filter[3][0]; //左2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, -1 * dy)) * filter[3][1]; //左
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -1 * dy)) * filter[3][2]; //中
    ret += tex.Sample(smp, output.uv + float2(1 * dx, -1 * dy)) * filter[3][3]; //右
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -1 * dy)) * filter[3][4]; //右2
    //最下段
    ret += tex.Sample(smp, output.uv + float2(-2 * dx, -2 * dy)) * filter[4][0]; //左2
    ret += tex.Sample(smp, output.uv + float2(-1 * dx, -2 * dy)) * filter[4][1]; //左
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -2 * dy)) * filter[4][2]; //中
    ret += tex.Sample(smp, output.uv + float2(1 * dx, -2 * dy)) * filter[4][3]; //右
    ret += tex.Sample(smp, output.uv + float2(0 * dx, -2 * dy)) * filter[4][4]; //右2
    
    return ret / 256;
}

float4 GaussianBlurX(Output output)
{
    float w, h, levels;
    tex.GetDimensions(0, w, h, levels);
    float dx = 1.f / w; //1ピクセル当たりの移動幅
    
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
    //平滑化フィルター
    float3x3 smoothing =
    {
        { 1 / 9, 1 / 9, 1 / 9 },
        { 1 / 9, 1 / 9, 1 / 9 },
        { 1 / 9, 1 / 9, 1 / 9 }
    };
    //エンボス加工
    float3x3 emboss =
    {
        { 2, 1, 0 },
        { 1, 1, -1 },
        { 0, -1, -2 }
    };
    //シャープネス(エッジの強調)
    float3x3 shapness =
    {
        { 0, -1, 0 },
        { -1, 5, -1 },
        { 0, -1, 0 }
    };
    //エッジ
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