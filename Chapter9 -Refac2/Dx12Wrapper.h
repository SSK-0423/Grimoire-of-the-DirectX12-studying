#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <wrl.h>

#include <vector>
#include <string>
#include <map>
#include <unordered_map>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

#define RETURN_FAILED(result) if(FAILED(result)){ assert(0); return result; }

class Dx12Wrapper {
private:
	HWND& _hwnd;
	SIZE windowSize;

	//DXGIまわり
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;

	//DirectX12まわり
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;

	//レンダーターゲット関連
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _backBuffers;	//バックバッファ
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH;

	//ビューポート＆シザー矩形
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorrect = {};

	//デプスステンシルビュー関連
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer = nullptr;	//深度バッファ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

	//シーンを構成するバッファまわり
	Microsoft::WRL::ComPtr<ID3D12Resource> _sceneConstBuff = nullptr;

	struct SceneData
	{
		DirectX::XMMATRIX view;	//ビュー行列
		DirectX::XMMATRIX proj;	//プロジェクション行列
		DirectX::XMFLOAT3 eye;//視点座標
	};
	SceneData* _mappedSceneData;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;

	//フェンス
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	//バリア
	D3D12_RESOURCE_BARRIER BarrierDesc = {};

	//デバイスとDXGIの生成
	HRESULT CreateDeviceAndDXGIFactory();
	//スワップチェーン生成
	HRESULT CreateSwapChain(HWND& hwnd, const SIZE& size);
	//コマンドアロケータ、リスト、キュー生成
	HRESULT CreateCommandX();
	//レンダーターゲットビュー生成
	HRESULT CreateFinalRenderTarget(const SIZE& size);
	////デプスステンシルビュー生成
	HRESULT CreateDepthStencilView(const SIZE& size);
	//フェンス作成
	HRESULT CreateID3D12Fence();

	//ビュープロジェクション用ビューの生成
	HRESULT	CreateSceneView();

	//テクスチャ読み込みのラムダ式
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> _loadLambdaTable;
	//テクスチャテーブル
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _textureTable;
	//テクスチャローダテーブル作成
	void CreateTextureLoaderTable();
	//ファイルからテクスチャ生成
	ID3D12Resource* CreateTextureFromFile(const char* texpath);

public:
	Dx12Wrapper(HWND& hwnd);
	~Dx12Wrapper();

	void Update();
	void EndDraw();
	void BeginDraw();

	///テクスチャパスから必要なテクスチャバッファへのポインタを返す
	///@param texpath テクスチャファイルパス
	Microsoft::WRL::ComPtr<ID3D12Resource> GetTextureByPath(const char* texPath);

	Microsoft::WRL::ComPtr<ID3D12Device> Device();	//デバイス
	Microsoft::WRL::ComPtr<IDXGISwapChain4> Swapchain();	//コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList();	//スワップチェーン

	void SetScene();
};