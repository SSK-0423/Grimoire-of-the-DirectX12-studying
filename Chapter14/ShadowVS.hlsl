#include "BasicType.hlsli"

//影用頂点座標変換(座標変換のみ)
//ライトカメラ視点
//便宜上、PMDと同様の引数にしている
float4 shadowVS(
	float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT) : SV_POSITION
{
    float fWeight = float(weight) / 100.f;
	
    matrix conBone = bones[boneno.x] * fWeight + bones[boneno.y] * (1.f - fWeight);
	
    pos = mul(world, mul(conBone, pos));
	
    return mul(lightCamera, pos);
}