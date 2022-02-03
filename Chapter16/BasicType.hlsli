Texture2D<float4> tex : register(t0); //0番スロットに設定されたテクスチャ(ベース)
Texture2D<float4> sph : register(t1); //1番スロットに設定されたテクスチャ(乗算)
Texture2D<float4> spa : register(t2); //2番スロットに設定されたテクスチャ(加算)
Texture2D<float4> toon : register(t3); //3番スロットに設定されたテクスチャ(トゥーン)
//シャドウマップ用ライド深度テクスチャ
Texture2D<float> lightDepthTex : register(t4);

SamplerState smp : register(s0); //0番スロットに設定されたサンプラ
SamplerState smpToon : register(s1); //1番スロットに設定されたサンプラ
SamplerComparisonState shadowSmp : register(s2);

//頂点シェーダ→ピクセルシェーダへのやり取りに使用する
//構造体
struct BasicType {
	float4 svpos:SV_POSITION;//システム用頂点座標
	float4 pos:POSITION;//頂点座標
	float4 normal:NORMAL0;//法線ベクトル
	float4 vnormal:NORMAL1;//法線ベクトル
	float2 uv:TEXCOORD;//UV値
	float3 ray:VECTOR;//ベクトル
    uint instNo : SV_InstanceID;
    float4 tpos : TPOS;
};

//定数バッファ0
cbuffer SceneData : register(b0)
{
    matrix view;
    matrix proj; //ビュープロジェクション行列
    matrix invPro;  //逆プロジェクション行列
    matrix lightCamera; //ライトビュープロジェクション
    matrix shadow; //影行列
    float3 eye;
};
cbuffer Transform : register(b1)
{
    matrix world; //ワールド変換行列
    matrix bones[256]; //ボーン行列 動的確保はできないためあらかじめ必要になりそうな数だけ確保しておく
}

//定数バッファ1
//マテリアル用
cbuffer Material : register(b2)
{
    float4 diffuse; //ディフューズ色
    float4 specular; //スペキュラ
    float3 ambient; //アンビエント
};

struct PixelOutput
{
    float4 col : SV_TARGET0;    //カラー値を出力
    float4 normal : SV_TARGET1; //法線を出力
	float4 highLum : SV_TARGET2;//高輝度(High Luminance)
};