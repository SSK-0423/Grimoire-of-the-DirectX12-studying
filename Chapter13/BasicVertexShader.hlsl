#include"BasicType.hlsli"

BasicType BasicVS(float4 pos : POSITION , float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT, uint instNo : SV_InstanceID) {
    float w = float(weight) / 100.f; //ウェイトは0～100を取るので、線形補間で扱えるように100で割る
	BasicType output;//ピクセルシェーダへ渡す値
	//渡されるボーン番号は2つ
    matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.f - w);
    pos = mul(bm,pos);	
	pos = mul(world, pos);
    if (instNo == 1)
    {
		//シャドウ行列を乗算
        pos = mul(shadow, pos);
    }
	
	output.svpos = mul(mul(proj,view),pos);//シェーダでは列優先なので注意
    //output.svpos = mul(lightCamera, pos);
	//シャドウマッピングができなかった原因
	//ライトから見た深度値とライトから見た座標を比較する(要確認)
	//ここでview、つまり正面カメラからの視点に切り替えてしまうと２つのカメラ変換がかかってしまうことになり
	//ライトから見た座標を正確に計算できなくなる
	//output.pos = mul(view, pos);	
    output.pos = pos;
	output.tpos = mul(lightCamera, output.pos);
	normal.w = 0;//ここ重要(平行移動成分を無効にする)
	output.normal = mul(world,normal);//法線にもワールド変換を行う
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - mul(view,eye));//視線ベクトル
    output.instNo = instNo;
	
	return output;
}
