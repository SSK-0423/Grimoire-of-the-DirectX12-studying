#pragma once
#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>

#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>
#include <wrl.h>
#include "Application.h"

#ifdef _DEBUG
#include<iostream>
#endif
#include <map>

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"

//関数マクロ
#define RETURN_FAILED(result) if(FAILED(result)){ assert(0); return result; }

class Application
{
private:
	//ここに必要な変数(バッファ―やヒープなど)を書く
	const unsigned int window_width = 1280;
	const unsigned int window_height = 720;

	HRESULT result;

	//ウィンドウ
	HWND hwnd;
	WNDCLASSEX windowClass;

	//DXGIまわり
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;

	//DirectX12まわり
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;

	//バッファ―
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer = nullptr;	//深度バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff = nullptr;		//頂点バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> idxBuff = nullptr;		//インデックスバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuff = nullptr;		//定数バッファ

	//レンダーターゲットビュー関連
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _backBuffers;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH;

	//ビューポート＆シザー矩形
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorrect = {};

	//デプスステンシルビュー関連
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

	//ビュー
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	//頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};	//インデックスバッファビュー

	//マテリアルまわり
	unsigned int materialNum;	// マテリアル数
	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> materialDescHeap = nullptr;

	//デフォルトテクスチャ
	Microsoft::WRL::ComPtr<ID3D12Resource> whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> gradTex = nullptr;

	//シェーダーオブジェクト
	Microsoft::WRL::ComPtr<ID3DBlob> _vsBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> _psBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	//テクスチャ読み込みのラムダ式
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> LoadLambdaTable;

	//ファイル名パスとリソースのマップテーブル
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _resourceTable;

	//ルートシグネチャ＆グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootsignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> basicDescHeap = nullptr;

	DirectX::XMMATRIX worldMat;
	DirectX::XMMATRIX viewMat;
	DirectX::XMMATRIX projMat;

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

	std::vector<Material> materials;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> toonResources;

	//シェーダ側に渡すための基本的な行列データ
	struct SceneMatrix {
		DirectX::XMMATRIX world;	//ワールド行列
		DirectX::XMMATRIX view;	//ビュー行列
		DirectX::XMMATRIX proj;	//プロジェクション行列
		DirectX::XMFLOAT3 eye;	//視点座標
	};
	SceneMatrix* mapMatrix;//マップ先を示すポインタ

	//フェンス
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

#pragma pack(push, 1)
	struct PMD_VERTEX
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
		uint16_t bone_no[2];
		uint8_t  weight;
		uint8_t  EdgeFlag;
		uint16_t dummy;
	};
#pragma pack(pop)

#pragma pack(1)	//アライメント抑制
	//PMDマテリアル構造体
	struct PMDMaterial
	{
		DirectX::XMFLOAT3 diffuse;	//ディフューズ
		float alpha;		//ディフューズα
		float specularity;	//スペキュラの強さ（乗算値）
		DirectX::XMFLOAT3 specular;	//スペキュラ色
		DirectX::XMFLOAT3 ambient;	//アンビエント色
		unsigned char toonIdx;	//トゥーン番号
		unsigned char edgeFlg;	//マテリアルごとの輪郭線フラグ
		//ここまで46
		//注意：ここで2バイトのパディングが入る

		unsigned int indicesNum;	//このマテリアルが割り当てられる
									//インデックス数
		char texFilePath[20];		//テクスチャファイルパス+α
	};//70バイトのはずだが、パディングが発生するため72バイトになる
#pragma pack()

	//PMDヘッダ構造体
	struct PMDHeader {
		float version; //例：00 00 80 3F == 1.00
		char model_name[20];//モデル名
		char comment[256];//モデルコメント
	};



	//DirectX12の初期化
	//ウィンドウ生成
	void CreateGameWindow();
	//デバイスとDXGIの生成
	HRESULT CreateDeviceAndDXGIFactory();
	//スワップチェーン生成
	HRESULT CreateSwapChain();
	//コマンドアロケータ、リスト、キュー生成
	HRESULT CreateCommandX();
	//レンダーターゲットビュー生成
	HRESULT CreateFinalRenderTarget();

	//デプスステンシルビュー生成
	HRESULT CreateDepthStencilView();
	//フェンス作成
	HRESULT CreateID3D12Fence();

	//PMD読み込み
	HRESULT LoadPMDFile(const char* path);

	//マテリアルデータ作成
	HRESULT CreateMaterialData();

	//シェーダーオブジェクト生成
	HRESULT CreateShaderObject();

	//ルートシグネチャ生成
	HRESULT CreateRootSig();

	//グラフィックスパイプライン生成
	HRESULT CreateBasicGraphicsPipeline();

	//座標変換用ビューの生成
	HRESULT CreateSceneTransformView();

	//デバッグ用
	void DebugOutputFormatString(const char* format, ...);

	//デバッグレイヤー有効化
	HRESULT EnableDebugLayer();

	//プロジェクトからテクスチャへの相対パス取得
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath);
	//拡張子取得
	std::string GetExtension(const std::string& path);
	//ファイルネーム分割
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');
	//stringからwstringへの変換
	std::wstring GetWideStringFromString(const std::string& str);

	//テクスチャファイル読み込み
	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath);
	//白テクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateWhiteTexture();
	//黒テクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBlackTexture();
	//グラデーションテクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateGrayGradationTexture();

	//テクスチャ読み込みのラムダ式テーブル作成
	void CreateTextureLoaderTable();

	//シングルトンのためにコンストラクタをprivateに
	//さらにコピーと代入を禁止する
	Application() {}
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;

public:
	//Applicationクラスのシングルトンインスタンスを得る
	static Application& Instance();

	//初期化
	bool Init();

	//ループ起動
	void Run();

	//後処理
	void Terminate();

	~Application();
};