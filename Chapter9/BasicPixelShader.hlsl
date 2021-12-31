#include"BasicType.hlsli"
Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); //0番スロットに設定されたサンプラ
SamplerState smpToon : register(s1); //1番スロットに設定されたサンプラー(トゥーン用)
Texture2D<float4> sph : register(t1); //1番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t2); //2番スロットに
Texture2D<float4> toon : register(t3);//3番スロットに設定されたテクスチャ(トゥーン)

//定数バッファ
cbuffer cbuff0 : register(b0)
{
    matrix world; //ワールド変換行列
    matrix view; //ビュー行列
    matrix proj; //プロジェクション行列
    float3 eye; //視点
};

cbuffer Material : register(b1)
{
    float4 diffuse; //(XMFLOAT3 + float)
    float4 specular; //(XMFLOAT3 + float)
    float3 ambient;
};

float4 BasicPS(BasicType input) : SV_TARGET
{
    //光の向かうベクトル（平行光線）
    float3 light = normalize(float3(1, -1, 1));
    
    //ライトのカラー（1,1,1で真っ白）
    float3 lightColor = float3(1, 1, 1);
    
    //ディフューズ計算
    float diffuseB = saturate(dot(-light, input.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));
    
    //光の反射ベクトル
    float3 refLight = normalize(reflect(light, input.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
    
    //スフィアマップ用UV
    float2 sphereMapUV = input.vnormal.xy; //スフィアマップ用法線
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);
    
    //テクスチャカラー
    float4 texColor = tex.Sample(smp, input.uv); //テクスチャカラー

    return max(saturate(toonDif //輝度
            * diffuse //ディフューズ色
            * texColor //テクスチャカラー
            * sph.Sample(smp, sphereMapUV)) //スフィアマップ(乗算)
            + spa.Sample(smp, sphereMapUV) * texColor //スフィアマップ(加算)
            + float4(specularB * specular.rgb, 1) //スペキュラ
            , float4(texColor * ambient, 1)); //アンビエント
    
    //return float4(brightness, brightness, brightness, 1) //輝度
    //    * diffuse //ディフューズ色
    //    * color
    //    * tex.Sample(smp, input.uv) // テクスチャカラー
    //    * sph.Sample(smp, sphereMapUV) // スフィアマップ(乗算)
    //    + spa.Sample(smp, sphereMapUV) // スフィアマップ(加算)
    //    + float4(color * ambient, 1);

    //return float4(brightness, brightness, brightness, 1) //輝度
    //    * diffuse //ディフューズ色
    //    * tex.Sample(smp, input.uv) // テクスチャカラー
    //    * sph.Sample(smp, normalUV) // スフィアマップ(乗算)
    //    + spa.Sample(smp, normalUV); // スフィアマップ(加算)
    

    //return float4(specularB * specular.rgb, 1);
	//return float4(input.normal.xyz,1);
	//return float4(tex.Sample(smp,input.uv));
}