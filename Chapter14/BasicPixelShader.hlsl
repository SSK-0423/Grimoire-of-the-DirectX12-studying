#include"BasicType.hlsli"

float4 BasicPS(BasicType input) : SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1)); //光の向かうベクトル(平行光線)
    float3 lightColor = float3(1, 1, 1); //ライトのカラー(1,1,1で真っ白)

	//ディフューズ計算
    float diffuseB = saturate(dot(-light, input.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//光の反射ベクトル
    float3 refLight = normalize(reflect(light, input.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);
	
	//スフィアマップ用UV
    float2 sphereMapUV = input.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	//テクスチャカラー
    float4 texColor = tex.Sample(smp, input.uv);
    //シャドウマップ無しの色
    float4 ret = saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor
		+ float4(specularB * specular.rgb, 1))
		+ float4(texColor.rgb * ambient * 0.5, 1);
	
	//
    float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
    
    float2 shadowUV = (posFromLightVP + float2(1, -1)) * float2(0.5, -0.5);

    //float depthFromLight = lightDepthTex.Sample(smp, shadowUV);
    float depthFromLight = lightDepthTex.SampleCmp(
		shadowSmp, //比較サンプラー
		shadowUV,	//uv値
		posFromLightVP.z - 0.005f);	//比較対象値
	
    float shadowWeight = 1.f;
    shadowWeight = lerp(0.5f, 1.f, depthFromLight);	//0.0になったら0.5になるように
    //if (depthFromLight < posFromLightVP.z - 0.001f)
    //    shadowWeight = 0.5f;
	
    if (input.instNo == 1)
    {
        return float4(0, 0, 0, 1);
    }
	/*
		どうやら、ライトから見たときの深度が取得できてないっぽい
		peraShaderのlightDepthを表示すると、しっかり取得できているので、
		描画時のディスクリプタ周りの設定がおかしいのかも→ルートシグネチャか？
		エラーは出てないので、バインドされていないことはなさそう
		真っ白に表示される→初期値になっている
		深度値の初期値をいじるとその値が反映されたのでディスクリプタ周りの設定はあってそう
		最終レンダリングで深度を初期化しているのがまずいか？←初期化してない
		シャドウパス(ライトデプス初期化)→1パス目(正面デプス初期化)→2パス目(1パス目からのテクスチャ表示)
	*/
	
    return float4(ret.rgb * shadowWeight, ret.a);
    return float4(depthFromLight, depthFromLight, depthFromLight, 1);
    return saturate(toonDif //輝度(トゥーン)
		* diffuse //ディフューズ色
		* texColor //テクスチャカラー
		* sph.Sample(smp, sphereMapUV)) //スフィアマップ(乗算)
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor //スフィアマップ(加算)
		+ float4(specularB * specular.rgb, 1)) //スペキュラー
		+ float4(texColor * ambient * 0.5, 1); //アンビエント(明るくなりすぎるので0.5にしてます)

    return float4(shadowWeight, shadowWeight, shadowWeight, 1);
}