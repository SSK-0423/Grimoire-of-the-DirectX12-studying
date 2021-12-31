#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

#include <d3dx12.h>
#pragma comment(lib,"d3d12.lib")

class Dx12Wrapper;
class PMDRenderer;

class PMDActor {
private:
	Dx12Wrapper& _dx12;
	PMDRenderer& _pmdRenderer;

	//バッファ―
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;		//頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;		//インデックスバッファ

	//ビュー
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};	//頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW _ibView = {};	//インデックスバッファビュー
	
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformMat = nullptr;//座標変換行列(今はワールドのみ)
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;//座標変換ヒープ
	
	//シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;
		float alpha;
		DirectX::XMFLOAT3 specular;
		float specularity;
		DirectX::XMFLOAT3 ambient;
	};

	// それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	// 全体をまとめるデータ
	struct Material
	{
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};
	
	//座標
	struct Transform
	{
		//内部に持っているXMMATRIXメンバーが16バイトアライメントであるため
		//Transformをnewする際には16バイト境界に確保する
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	Transform _transform;
	Transform* _mappedTransform = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformBuff = nullptr;

	//マテリアル関連
	std::vector<Material> _materials;
	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;

	//読み込んだマテリアルをもとにマテリアルバッファを作成
	HRESULT CreateMaterialData();
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialHeap = nullptr;
	//マテリアル＆テクスチャのびゅーを作成
	HRESULT CreateMaterialAndTextureView();

	//座標変換用ビューの生成
	HRESULT CreateTransformView();

	//PMD読み込み
	HRESULT LoadPMDFile(const char* path);

	float _angle;//テスト用Y軸回転

public:
	PMDActor(const char* path, PMDRenderer& pmdRenderer, Dx12Wrapper& dx12);
	~PMDActor();
	//クローンは頂点およびマテリアルは共通のバッファを見るようにする
	PMDActor* Clone();
	void Init();
	void Update();
	void Draw();
};