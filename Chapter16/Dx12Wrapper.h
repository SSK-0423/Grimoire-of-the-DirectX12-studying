#pragma once
#include<d3d12.h>
#include<dxgi1_6.h>
#include<map>
#include<memory>
#include<unordered_map>
#include<DirectXTex.h>
#include<wrl.h>
#include<string>
#include<functional>
#include<array>

class PMDRenderer;

class Dx12Wrapper
{
	SIZE _winSize;
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//DXGIまわり
	ComPtr < IDXGIFactory4> _dxgiFactory = nullptr;//DXGIインターフェイス
	ComPtr < IDXGISwapChain4> _swapchain = nullptr;//スワップチェイン

	//DirectX12まわり
	ComPtr< ID3D12Device> _dev = nullptr;//デバイス
	ComPtr < ID3D12CommandAllocator> _cmdAllocator = nullptr;//コマンドアロケータ
	ComPtr < ID3D12GraphicsCommandList> _cmdList = nullptr;//コマンドリスト
	ComPtr < ID3D12CommandQueue> _cmdQueue = nullptr;//コマンドキュー

	//表示に関わるバッファ周り
	std::vector<ID3D12Resource*> _backBuffers;//バックバッファ(2つ以上…スワップチェインが確保)
	ComPtr<ID3D12DescriptorHeap> _rtvHeaps = nullptr;//レンダーターゲット用デスクリプタヒープ
	float _bgColor[4] = { 0.5f,0.5f,0.5f,1.f };

	//深度関連
	ComPtr<ID3D12Resource> _depthBuffer = nullptr;//深度バッファ
	ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;//深度バッファビュー用デスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> _depthSRVHeap = nullptr;
	//シャドウマップ用深度バッファ―
	ComPtr<ID3D12Resource> _lightDepthBuffer = nullptr;
	//影用パイプライン
	ComPtr<ID3D12PipelineState> _shadowPipline = nullptr;                                                                                                                                                                                     

	std::unique_ptr<D3D12_VIEWPORT> _viewport;//ビューポート
	std::unique_ptr<D3D12_RECT> _scissorrect;//シザー矩形

	//シーンを構成するバッファまわり
	ComPtr<ID3D12Resource> _sceneConstBuff = nullptr;

	struct SceneData {
		DirectX::XMMATRIX view;	//ビュー行列
		DirectX::XMMATRIX proj;	//プロジェクション行列
		DirectX::XMMATRIX invProj;	//逆プロジェクション行列
		DirectX::XMMATRIX lightCamera;	//ライトから見たビュー
		DirectX::XMMATRIX shadow;	//影
		DirectX::XMFLOAT3 eye;//視点座標
	};
	SceneData* _mappedSceneData;
	ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;
	//平行ライトの向き
	DirectX::XMFLOAT3 _perallelLightvVec;
	//Fov
	float _fov = DirectX::XM_PIDIV4;

	//フェンス
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	struct PeraVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	PeraVertex pv[4] = {
		{ {-1,-1,0.1}	,{0,1}},	//左下
		{ {-1,1,0.1}	,{0,0}},	//左上
		{ {1,-1,0.1}	,{1,1}},	//右下
		{ {1,1,0.1}		,{1,0}} };	//右上
	ComPtr<ID3D12Resource> _peraVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _peraVBV;

	ComPtr<ID3D12RootSignature> _peraRootSignature = nullptr;
	ComPtr<ID3D12PipelineState> _peraPipeline = nullptr;
	ComPtr<ID3D12RootSignature> _peraRootSignature2 = nullptr;
	ComPtr<ID3D12PipelineState> _peraPipeline2 = nullptr;

	//1枚目のレンダリング結果
	//ペラポリゴン
	std::array<ComPtr<ID3D12Resource>,2> _peraResource;
	ComPtr<ID3D12Resource> _peraResource2 = nullptr;
	ComPtr<ID3D12DescriptorHeap> _peraRTVHeap = nullptr;	//レンダーターゲット用
	ComPtr<ID3D12DescriptorHeap> _peraSRVHeap = nullptr;	//テクスチャ用

	HRESULT CreatePeraResourceAndView();
	//ペラポリゴンの頂点バッファ―＆ビュー生成
	HRESULT CreatePeraVertexBufferAndView();
	//ペラポリゴン用のパイプライン生成
	HRESULT CreatePeraPipeline();

	//最終的なレンダーターゲットの生成
	HRESULT	CreateFinalRenderTargets();
	//デプスステンシルビューの生成
	HRESULT CreateDepthStencilView();
	//デプス用SRV生成
	HRESULT CreateDepthSRV();
	//影用パイプライン生成
	HRESULT CreateShadowPipeline();

	//スワップチェインの生成
	HRESULT CreateSwapChain(const HWND& hwnd);

	//DXGIまわり初期化
	HRESULT InitializeDXGIDevice();

	//コマンドまわり初期化
	HRESULT InitializeCommand();

	//ビュープロジェクション用ビューの生成
	HRESULT CreateSceneView();

	//ロード用テーブル
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map < std::string, LoadLambda_t> _loadLambdaTable;
	//テクスチャテーブル
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> _textureTable;
	//テクスチャローダテーブルの作成
	void CreateTextureLoaderTable();
	//テクスチャ名からテクスチャバッファ作成、中身をコピー
	ID3D12Resource* CreateTextureFromFile(const char* texpath);

	//ガウシアンフィルタ関連
	ComPtr<ID3D12Resource> _gaussianBuff = nullptr;
	std::vector<float> _gaussianWeights;
	std::vector<float> GetGaussianWeights(size_t count, float s);
	HRESULT CreateGaussianConstantBufferAndView();

	//歪みテクスチャ用
	ComPtr<ID3D12DescriptorHeap> _effectSRVHeap = nullptr;
	ComPtr<ID3D12Resource> _effectTexBuffer = nullptr;
	HRESULT CreateEffectBufferAndView();

	//ブルーム
	std::array<ComPtr<ID3D12Resource>, 2> _bloomBuffer;	//ブルーム用バッファー
	ComPtr<ID3D12DescriptorHeap> _bloomHeap = nullptr;
	HRESULT CreateBloomBufferAndView();
	//画面全体ぼかし用パイプライン
	ComPtr<ID3D12PipelineState> _blurPipeline;
	//被写界深度用ぼかしバッファー
	ComPtr<ID3D12Resource> _dofBuffer;
	HRESULT CreateDofBuffer();

	//アンビエントオクルージョン関連
	ComPtr<ID3D12Resource> _aoBuffer = nullptr;
	ComPtr<ID3D12PipelineState> _aoPipeline = nullptr;
	ComPtr<ID3D12DescriptorHeap> _aoRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> _aoSRVHeap = nullptr;
	HRESULT CreateAmbientOcclusionBuffer();
	HRESULT CreateAmbientOcclusionDescriptorHeap();

	// imgui用のヒープ生成
	ComPtr<ID3D12DescriptorHeap> _heapForImgui = nullptr;	//ヒープ保持用
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeapForImgui();

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	//シャドウのレンダリング
	void PreDrawShadow();
	void EndDrawShadow();

	//1パス目のリンダリング
	void PreDrawRenderTarget1();
	void DrawRenderTarget1(std::shared_ptr<PMDRenderer>& renderer);
	void EndDrawRenderTarget1();
	//2パス目のレンダリング
	void PreDrawRenderTarget2();
	void DrawRenderTarget2();
	void EndDrawRenderTarget2();

	//最終レンダリング
	void PreDrawFinalRenderTarget();
	//最終レンダリング結果描画
	void DrawFinalRenderTarget();
	//縮小バッファーへの書き込み
	void DrawShrinkTextureForBlur();
	//アンビエントオクルージョン描画
	void DrawAmbientOcculusion();

	void EndDraw();
	void BeginDraw();
	///テクスチャパスから必要なテクスチャバッファへのポインタを返す
	///@param texpath テクスチャファイルパス
	ComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);

	ComPtr< ID3D12Device> Device();//デバイス
	ComPtr < ID3D12GraphicsCommandList> CommandList();//コマンドリスト
	ComPtr < IDXGISwapChain4> Swapchain();//スワップチェイン
	ComPtr<ID3D12DescriptorHeap> GetHeapForImgui();	//ImGui用のヒープ取得
	void SetScene();
	//シーン設定
	void SetCameraSetting();

	//Imguiによる操作反映
	void SetDebugDisplay(bool flg);	//デバッグ表示のON / OFF
	void SetSSAO(bool flg);			//SSAOのON / OFF
	void SetSelfShadow(bool flg);	//セルフシャドウ ON / OFF
	void SetFov(float fov);			//画角(30°～150°)
	void SetLightVector(float vec[3]);	//光線ベクトル(xyzベクトル)
	void SetBackColor(float col[4]);	//背景色の変更
	void SetBloomColor(float col[3]);	//ブルームの色付け
};

