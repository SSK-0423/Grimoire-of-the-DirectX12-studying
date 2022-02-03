#include "peraHeader.hlsli"

//SSAOの処理のためだけのシェーダー
Texture2D<float4> normaltex : register(t1);//1パス目の法線描画
Texture2D<float> depthtex : register(t6);   //１パス目の深度テクスチャ

//現在のUV値を元に乱数を返す
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}
//SSAO(乗算用の明度のみ情報を返せればよい)
float SsaoPs(Output input) : SV_TARGET
{
    float dp = depthtex.Sample(smp, input.uv);  //現在の深度
    
    float w, h, miplevels;
    depthtex.GetDimensions(0, w, h, miplevels);
    float dx = 1.f / w;
    float dy = 1.f / h;
    
    //SSAO
    //2.元の座標を復元する
    float4 respos = mul(invProj, float4(input.uv * float2(2, -2) + float2(-1, 1), dp, 1));
    
    respos.xyz = respos.xyz / respos.w;
    
    float div = 0.f;
    float ao = 0.f;
    float3 norm = normalize((normaltex.Sample(smp, input.uv).xyz * 2) - 1);
    const int trycnt = 256;
    const float radius = 0.5f;
    
    // 深度が1以下=カメラの範囲内
    if(dp < 1.f)
    {
        //3.サンプル回数繰り返す
        for (int i = 0; i < trycnt; i++)
        {
            //3.1 ランダム方向(大きさは固定)にベクトルを飛ばす
            float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
            float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
            float rnd3 = random(float2(rnd2, rnd1)) * 2 - 1;
            float3 omega = normalize(float3(rnd1, rnd2, rnd3));
            omega = normalize(omega);
            
            // 乱数の結果法線の反対側に向いていたら反転する
            float dt = dot(norm, omega);
            float sgn = sign(dt);   //符号取得
            omega *= sign(dt);
            
            // 結果の座標を再び射影変換する
            // 3.2 
            float4 rpos = mul(proj, float4(respos.xyz + omega * radius, 1));
            rpos.xyz /= rpos.w;
            dt *= sgn;  // 正の値にしてcosθを得る
            div += dt;  // 遮蔽を考えない結果を加算する
            
            // 計算結果が現在の場所の深度より奥に入っているなら
            // 遮蔽されているので加算
            ao += step(depthtex.Sample(smp, (rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f)), rpos.z) * dt;
        }
        ao /= div;
    }
    return 1.f - ao;
}