#include "Dx12Wrapper.h"
#include <cassert>
#include <d3dx12.h>
#include "Application.h"
#include "PMDRenderer.h"
#include "Dx12Helper.h"
#include "DirectXTex.h"

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace {
	constexpr uint32_t shadow_difinition = 1024;
	
	///モデルのパスとテクスチャのパスから合成パスを得る
	///@param modelPath アプリケーションから見たpmdモデルのパス
	///@param texPath PMDモデルから見たテクスチャのパス
	///@return アプリケーションから見たテクスチャのパス
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//ファイルのフォルダ区切りは\と/の二種類が使用される可能性があり
		//ともかく末尾の\か/を得られればいいので、双方のrfindをとり比較する
		//int型に代入しているのは見つからなかった場合はrfindがepos(-1→0xffffffff)を返すため
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}

	///ファイル名から拡張子を取得する
	///@param path 対象のパス文字列
	///@return 拡張子
	string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///ファイル名から拡張子を取得する(ワイド文字版)
	///@param path 対象のパス文字列
	///@return 拡張子
	wstring
		GetExtension(const std::wstring& path) {
		int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///テクスチャのパスをセパレータ文字で分離する
	///@param path 対象のパス文字列
	///@param splitter 区切り文字
	///@return 分離前後の文字列ペア
	pair<string, string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		pair<string, string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}

	///string(マルチバイト文字列)からwstring(ワイド文字列)を得る
	///@param str マルチバイト文字列
	///@return 変換されたワイド文字列
	std::wstring
		GetWideStringFromString(const std::string& str) {
		//呼び出し1回目(文字列数を得る)
		auto num1 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, nullptr, 0);

		std::wstring wstr;//stringのwchar_t版
		wstr.resize(num1);//得られた文字列数でリサイズ

		//呼び出し2回目(確保済みのwstrに変換文字列をコピー)
		auto num2 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, &wstr[0], num1);

		assert(num1 == num2);//一応チェック
		return wstr;
	}
	///デバッグレイヤーを有効にする
	void EnableDebugLayer() {
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
	}
}
HRESULT Dx12Wrapper::CreateEffectBufferAndView()
{
	//法線マップをロードする
	_effectTexBuffer = CreateTextureFromFile("normal/crack_n.png");
	if (_effectTexBuffer == nullptr) {
		assert(0);
		return E_FAIL;
	}

	//ポストエフェクト用のディスクリプタヒープ生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_effectSRVHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	//ポストエフェクト用テクスチャビューを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = _effectTexBuffer->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	_dev->CreateShaderResourceView(_effectTexBuffer.Get(), &srvDesc, _effectSRVHeap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

//ブルーム用リソース作成
HRESULT Dx12Wrapper::CreateBloomBufferAndView()
{
	auto& bbuff = _backBuffers[0];
	auto resDesc = bbuff->GetDesc();
	D3D12_HEAP_PROPERTIES heapPrpp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = 0.f;
	clearValue.Color[3] = 1.f;
	clearValue.Format = resDesc.Format;
	HRESULT result = S_OK;
	for (auto& buff : _bloomBuffer)
	{
		result = _dev->CreateCommittedResource(
			&heapPrpp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(buff.ReleaseAndGetAddressOf())
		);
		resDesc.Width >>= 1;	//右シフト(1/2)
		if (FAILED(result))
		{
			assert(0);
			return result;
		}
	}
	return result;
}
HRESULT Dx12Wrapper::CreateDofBuffer()
{
	auto& bbuff = _backBuffers[0];
	auto resDesc = bbuff->GetDesc();
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = 0.5f;
	clearValue.Color[3] = 1.f;
	clearValue.Format = resDesc.Format;
	resDesc.Width >>= 1;	//縮小バッファーなので大きさを半分にする
	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_dofBuffer.ReleaseAndGetAddressOf())
	);

	return result;
}
Dx12Wrapper::Dx12Wrapper(HWND hwnd) : _perallelLightvVec(1, -1, 1) {
#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif

	auto& app = Application::Instance();
	_winSize = app.GetWindowSize();

	//テクスチャローダー関連初期化
	CreateTextureLoaderTable();

	//DirectX12関連初期化
	if (FAILED(InitializeDXGIDevice())) {
		assert(0);
		return;
	}
	if (FAILED(InitializeCommand())) {
		assert(0);
		return;
	}
	if (FAILED(CreateSwapChain(hwnd))) {
		assert(0);
		return;
	}
	if (FAILED(CreateFinalRenderTargets())) {
		assert(0);
		return;
	}

	if (FAILED(CreateSceneView())) {
		assert(0);
		return;
	}

	//ブルームのバッファー生成
	if (FAILED(CreateBloomBufferAndView()))
	{
		assert(0);
		return;
	}
	//被写界深度のバッファー生成
	if (FAILED(CreateDofBuffer()))
	{
		assert(0);
		return;
	}

	if (FAILED(CreatePeraResourceAndView())) {
		assert(0);
		return;
	}
	if (FAILED(CreatePeraVertexBufferAndView()))
	{
		assert(0);
		return;
	}
	if (FAILED(CreateGaussianConstantBufferAndView()))
	{
		assert(0);
		return;
	}
	if (FAILED(CreateEffectBufferAndView()))
	{
		assert(0);
		return;
	}
	if (FAILED(CreatePeraPipeline()))
	{
		assert(0);
		return;
	}

	//深度バッファ作成
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return;
	}

	//深度値をテクスチャとして利用する
	if (FAILED(CreateDepthSRV()))
	{
		assert(0);
		return;
	}
	
	//if (FAILED(CreateShadowPipeline()))
	//{
	//	assert(0);
	//	return;
	//}

	//フェンスの作成
	if (FAILED(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())))) {
		assert(0);
		return;
	}

}

HRESULT
Dx12Wrapper::CreateDepthStencilView() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	if (FAILED(result)) {
		return result;
	}
	//深度バッファ作成
	//深度バッファの仕様
	//auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
	//	desc.Width, desc.Height,
	//	1, 0, 1, 0,
	//	D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resdesc.DepthOrArraySize = 1;
	resdesc.Width = desc.Width;
	resdesc.Height = desc.Height;
	resdesc.Format = DXGI_FORMAT_R32_TYPELESS;	//ビット数のみ指定
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resdesc.MipLevels = 1;
	resdesc.Alignment = 0;

	//デプス用ヒーププロパティ
	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.f, 0);
	//深度バッファ―生成
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //デプス書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		//エラー処理
		return result;
	}

	//シャドウマップ用の深度バッファ―生成
	resdesc.Width = shadow_difinition;
	resdesc.Height = shadow_difinition;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //デプス書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(_lightDepthBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		//エラー処理
		return result;
	}

	//深度のためのデスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使うよという事がわかればいい
	dsvHeapDesc.NumDescriptors = 2;//通常＋シャドウマップ用深度
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//デプスステンシルビューとして使う
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//深度用のディスクリプタヒープ生成
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return result;
	}

	//通常深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//デプス値に32bit使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//フラグは特になし

	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, handle);
	//ライト深度生成
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_dev->CreateDepthStencilView(_lightDepthBuffer.Get(), &dsvDesc, handle);


	return result;
}

//SRV用のデプス
HRESULT Dx12Wrapper::CreateDepthSRV()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 2;
	heapDesc.NodeMask = 0;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//深度SRVのディスクリプタヒープ生成
	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_depthSRVHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return result;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//通常深度→テクスチャ用
	auto handle = _depthSRVHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(_depthBuffer.Get(), &srvDesc, handle);

	//ライト深度→テクスチャ用
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_dev->CreateShaderResourceView(_lightDepthBuffer.Get(), &srvDesc, handle);

	return result;
}


Dx12Wrapper::~Dx12Wrapper()
{
}

ComPtr<ID3D12Resource>
Dx12Wrapper::GetTextureByPath(const char* texpath) {
	auto it = _textureTable.find(texpath);
	if (it != _textureTable.end()) {
		//テーブルに内にあったらロードするのではなくマップ内の
		//リソースを返す
		return _textureTable[texpath];
	}
	else {
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texpath));
	}

}

//テクスチャローダテーブルの作成
void
Dx12Wrapper::CreateTextureLoaderTable() {
	_loadLambdaTable["sph"] = _loadLambdaTable["spa"] = _loadLambdaTable["bmp"] = _loadLambdaTable["png"] = _loadLambdaTable["jpg"] = [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	_loadLambdaTable["tga"] = [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	_loadLambdaTable["dds"] = [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

//テクスチャ名からテクスチャバッファ作成、中身をコピー
ID3D12Resource*
Dx12Wrapper::CreateTextureFromFile(const char* texpath) {
	string texPath = texpath;
	//テクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = GetWideStringFromString(texPath);//テクスチャのファイルパス
	auto ext = GetExtension(texPath);//拡張子を取得
	auto result = _loadLambdaTable[ext](wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return nullptr;
	}
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出

	//WriteToSubresourceで転送する用のヒープ設定
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	if (FAILED(result)) {
		return nullptr;
	}
	result = texbuff->WriteToSubresource(0,
		nullptr,//全領域へコピー
		img->pixels,//元データアドレス
		img->rowPitch,//1ラインサイズ
		img->slicePitch//全サイズ
	);
	if (FAILED(result)) {
		return nullptr;
	}

	return texbuff;
}

void Dx12Wrapper::SetCameraSetting()
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	if (FAILED(result))
	{
		assert(0);
		return;
	}

	XMFLOAT3 eye(0, 15, -25);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	XMFLOAT4 planeNormalVec(0, 1, 0, 0);	// 平面の方程式
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は45°
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//アス比
		1.f,//近い方
		100.0f//遠い方
	);

	auto eyePos = XMLoadFloat3(&eye);
	auto targetPos = XMLoadFloat3(&target);
	auto upVec = XMLoadFloat3(&up);

	auto light = XMFLOAT4(-1, 1, -1, 0);
	XMVECTOR lightVec = XMLoadFloat4(&light);
	auto lightPos = targetPos + XMVector3Normalize(lightVec) * XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_mappedSceneData->lightCamera = XMMatrixLookAtLH(lightPos, targetPos, upVec) * XMMatrixOrthographicLH(40, 40, 1.f, 100.f);
	_mappedSceneData->shadow = XMMatrixShadow(XMLoadFloat4(&planeNormalVec), -XMLoadFloat3(&_perallelLightvVec));	//影行列作成
	_mappedSceneData->eye = eye;
}

std::vector<float> Dx12Wrapper::GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);	//ウェイト配列返却用
	float x = 0.f;
	float total = 0.f;
	for (auto& wgt : weights)
	{
		wgt = expf(-(x * x) / (2 * s * s));
		total += wgt;
		x += 1.f;
	}

	total = total * 2.f - 1;
	//足して1になるようにする
	for (auto& wgt : weights)
	{
		wgt /= total;
	}

	return weights;
}

//
// ヒーププロパティ設定
// リソース設定
// 生成
// マップ
// ビュー設定
// ビュー生成
//
HRESULT Dx12Wrapper::CreateGaussianConstantBufferAndView()
{
	_gaussianWeights = GetGaussianWeights(8, 5);
	//バッファ―生成
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(
		AlignmentedSize(_gaussianWeights.size() * sizeof(float), 256));
	auto result = _dev->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(_gaussianBuff.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	//マップ処理
	float* mappedWeight = nullptr;
	result = _gaussianBuff->Map(0, nullptr, (void**)&mappedWeight);
	if (FAILED(result))
	{
		assert(0);
		return result;
	}
	std::copy(_gaussianWeights.begin(), _gaussianWeights.end(), mappedWeight);
	_gaussianBuff->Unmap(0, nullptr);

	//ビュー生成
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
	cbv.BufferLocation = _gaussianBuff->GetGPUVirtualAddress();
	cbv.SizeInBytes = _gaussianBuff->GetDesc().Width;

	//CreatePeraResourceAndViewメソッドを変更した際は注意
	auto handle = _peraSRVHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 6;
	_dev->CreateConstantBufferView(&cbv, handle);

	return result;
}

HRESULT
Dx12Wrapper::InitializeDXGIDevice() {
	UINT flagsDXGI = 0;
	flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
	//DirectX12まわり初期化
	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	if (FAILED(result)) {
		return result;
	}
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}
	result = S_FALSE;
	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (SUCCEEDED(D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())))) {
			featureLevel = l;
			result = S_OK;
			break;
		}
	}
	return result;
}


///スワップチェイン生成関数
HRESULT
Dx12Wrapper::CreateSwapChain(const HWND& hwnd) {
	RECT rc = {};
	::GetWindowRect(hwnd, &rc);


	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = _winSize.cx;
	swapchainDesc.Height = _winSize.cy;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	auto result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue.Get(),
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(result));
	return result;
}

//コマンドまわり初期化
HRESULT
Dx12Wrapper::InitializeCommand() {
	auto result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//プライオリティ特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//ここはコマンドリストと合わせてください
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));//コマンドキュー生成
	assert(SUCCEEDED(result));
	return result;
}

//ビュープロジェクション用ビューの生成
HRESULT
Dx12Wrapper::CreateSceneView() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	//定数バッファ作成
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_sceneConstBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	_mappedSceneData = nullptr;//マップ先を示すポインタ
	result = _sceneConstBuff->Map(0, nullptr, (void**)&_mappedSceneData);//マップ

	XMFLOAT3 eye(0, 15, -25);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	XMFLOAT4 planeNormalVec(0, 1, 0, 0);	// 平面の方程式
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は45°
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//アス比
		1.f,//近い方
		100.0f//遠い方
	);

	auto eyePos = XMLoadFloat3(&eye);
	auto targetPos = XMLoadFloat3(&target);
	auto upVec = XMLoadFloat3(&up);

	auto light = XMFLOAT4(-1, 1, -1, 0);
	XMVECTOR lightVec = XMLoadFloat4(&light);
	auto lightPos = targetPos + XMVector3Normalize(lightVec) * XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_mappedSceneData->lightCamera = XMMatrixLookAtLH(lightPos,targetPos,upVec) * XMMatrixOrthographicLH(40, 40, 1.f, 100.f);
	_mappedSceneData->shadow = XMMatrixShadow(XMLoadFloat4(&planeNormalVec), -XMLoadFloat3(&_perallelLightvVec));	//影行列作成
	_mappedSceneData->eye = eye;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	descHeapDesc.NodeMask = 0;//マスクは0
	descHeapDesc.NumDescriptors = 1;//
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_sceneDescHeap.ReleaseAndGetAddressOf()));//生成

	////デスクリプタの先頭ハンドルを取得しておく
	auto heapHandle = _sceneDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _sceneConstBuff->GetDesc().Width;
	//定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, heapHandle);
	return result;

}

HRESULT Dx12Wrapper::CreateShadowPipeline()
{
	ComPtr<ID3DBlob> vsBlob;
	ComPtr<ID3DBlob> errBlob;
	auto result = D3DCompileFromFile(L"ShadowVS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"shadowVS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC plsDesc = {};
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());	//頂点シェーダー設定
	//plsDesc.PS.BytecodeLength = 0;	//ピクセルシェーダー必要なし
	//plsDesc.PS.pShaderBytecode = nullptr; //ピクセルシェーダー必要なし
	plsDesc.NumRenderTargets = 1;
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;;
	plsDesc.pRootSignature = _peraRootSignature.Get();
	plsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	result = _dev->CreateGraphicsPipelineState(&plsDesc, IID_PPV_ARGS(_shadowPipline.ReleaseAndGetAddressOf()));

	return result;
}

HRESULT Dx12Wrapper::CreatePeraResourceAndView()
{
	//使っているバックバッファ―の情報を利用する
	auto& bbuff = _backBuffers[0];
	D3D12_RESOURCE_DESC resDesc = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//レンダリング時のクリア値と同じ値
	float clsClr[] = { 0.5f,0.5f,0.5f,1.0f };//白色
	//float clsClr[] = { 0.f,0.f,0.f,1.0f };//白色
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);

	//1パス目はマルチレンダーターゲット
	for (auto& pera : _peraResource)
	{
		auto result = _dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			//D3D12_RESOURCE_STATE_RENDER_TARGETではない
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(pera.ReleaseAndGetAddressOf())
		);

		if (FAILED(result)) {
			assert(0);
			return result;
		}
	}

	//2パス目
	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		//D3D12_RESOURCE_STATE_RENDER_TARGETではない
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_peraResource2.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//RTV用ヒープを作る
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = _rtvHeaps->GetDesc();
	heapDesc.NumDescriptors = 6;	//1パス目(4枚) 2パス目(1枚)
	result = _dev->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(_peraRTVHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//レンダーターゲットビューを作る
	//1,2枚目
	auto handle = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& pera : _peraResource)
	{
		_dev->CreateRenderTargetView(
			pera.Get(), &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	//3枚目(ブルーム)
	_dev->CreateRenderTargetView(
		_bloomBuffer[0].Get(), &rtvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	//4枚目(縮小バッファー)
	_dev->CreateRenderTargetView(
		_bloomBuffer[1].Get(), &rtvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//5枚目(被写界深度)
	_dev->CreateRenderTargetView(
		_dofBuffer.Get(), &rtvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//6枚目
	_dev->CreateRenderTargetView(
		_peraResource2.Get(), &rtvDesc, handle);

	//SRV用ヒープを作る
	//1～5ペラポリゴン1用 6ペラポリゴン2用 + ガウスぼかし
	heapDesc.NumDescriptors = 7;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	result = _dev->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(_peraSRVHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	//シェーダーリソースビューを作る
	//1パス目(1,2枚)
	handle = _peraSRVHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& pera : _peraResource)
	{
		_dev->CreateShaderResourceView(
			pera.Get(), &srvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	//3枚目(ブルーム)
	_dev->CreateShaderResourceView(
		_bloomBuffer[0].Get(), &srvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//4枚目(縮小バッファー)
	_dev->CreateShaderResourceView(
		_bloomBuffer[1].Get(), &srvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//5枚目(被写界深度用縮小バッファー)
	_dev->CreateShaderResourceView(
		_dofBuffer.Get(), &srvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//2パス目(6枚目)
	_dev->CreateShaderResourceView(
		_peraResource2.Get(), &srvDesc, handle);

	return result;
}

HRESULT Dx12Wrapper::CreatePeraVertexBufferAndView()
{
	//頂点バッファ―作成
	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv));

	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_peraVB.ReleaseAndGetAddressOf())
	);

	//頂点バッファ―ビュー作成
	_peraVBV.BufferLocation = _peraVB->GetGPUVirtualAddress();
	_peraVBV.SizeInBytes = sizeof(pv);
	_peraVBV.StrideInBytes = sizeof(pv[0]);

	//マップ
	PeraVertex* mapped = nullptr;
	_peraVB->Map(0, nullptr, (void**)&mapped);
	std::copy(std::begin(pv), std::end(pv), mapped);
	_peraVB->Unmap(0, nullptr);

	return result;
}

HRESULT Dx12Wrapper::CreatePeraPipeline()
{
	//頂点レイアウト設定
	D3D12_INPUT_ELEMENT_DESC layout[2] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	//パイプラインステートの設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errBlob;
	//頂点シェーダーコンパイル
	auto result = D3DCompileFromFile(
		L"peraVertex.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs", "vs_5_0",
		0, 0, vs.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	//ピクセルシェーダーコンパイル
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps", "ps_5_0",
		0, 0, ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	D3D12_DESCRIPTOR_RANGE range[4] = {};
	//前パスのレンダリング結果
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range[0].BaseShaderRegister = 0;	// t0～t4
	range[0].NumDescriptors = 5; //マルチレンダーターゲット(3枚)なので3

	//エフェクト用バッファー
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	// b
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;

	//エフェクト用のテクスチャ
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range[2].BaseShaderRegister = 5;	// 1
	range[2].NumDescriptors = 1;
	
	//深度表示用テクスチャ
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range[3].BaseShaderRegister = 6;	//2
	range[3].NumDescriptors = 2;	// 使うのは1個

	D3D12_ROOT_PARAMETER rootParam[4] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParam[1].DescriptorTable.pDescriptorRanges = &range[1];
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParam[2].DescriptorTable.pDescriptorRanges = &range[2];
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;

	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParam[3].DescriptorTable.pDescriptorRanges = &range[3];
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);	// s0

	//ルートシグネチャ生成
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.NumParameters = _countof(rootParam);
	rsDesc.pParameters = &rootParam[0];
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &sampler;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rsBlob;

	result = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		rsBlob.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	result = _dev->CreateRootSignature(
		0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(),
		IID_PPV_ARGS(_peraRootSignature.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	gpsDesc.pRootSignature = _peraRootSignature.Get();
	//パイプラインステート生成
	//1枚目
	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//2枚目
	//ピクセルシェーダーコンパイル
	result = D3DCompileFromFile(
		L"VerticalBokehPS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VerticalBokehPS", "ps_5_0",
		0, 0, ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	//パイプラインステート生成
	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline2.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//ぼかし用パイプライン生成
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BlurPS", "ps_5_0",
		0, 0, ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		assert(0);
		return result;
	}
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	//ブルーム縮小と被写界深度縮小
	gpsDesc.NumRenderTargets = 2;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_blurPipeline.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT
Dx12Wrapper::CreateFinalRenderTargets() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		SUCCEEDED(result);
		return result;
	}
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	_backBuffers.resize(swcDesc.BufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; ++i) {
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
		assert(SUCCEEDED(result));
		rtvDesc.Format = _backBuffers[i]->GetDesc().Format;
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	_viewport.reset(new CD3DX12_VIEWPORT(_backBuffers[0]));
	_scissorrect.reset(new CD3DX12_RECT(0, 0, desc.Width, desc.Height));
	return result;
}

ComPtr< ID3D12Device>
Dx12Wrapper::Device() {
	return _dev;
}
ComPtr < ID3D12GraphicsCommandList>
Dx12Wrapper::CommandList() {
	return _cmdList;
}

void
Dx12Wrapper::Update() {

}

//1パス目の描画準備(通常描画)
void Dx12Wrapper::PreDrawRenderTarget1()
{
	//バリア設定
	for (auto& pera : _peraResource)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pera.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		_cmdList->ResourceBarrier(1, &barrier);
	}
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_bloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE handles[3];
	D3D12_CPU_DESCRIPTOR_HANDLE baseHandle = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//レンダーターゲットセット
	uint32_t offset = 0;
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseHandle, offset);
		offset += incSize;
	}

	//深度を指定
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//1パス目のレンダーターゲットセット
	//正面からの深度セット
	_cmdList->OMSetRenderTargets(_countof(handles), handles, false, &dsvH);	//深度値が書き込まれるようになる

	//画面クリア
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//白色
	for (size_t i = 0; i < _countof(handles); i++)
	{
		//高輝度の場合
		if (i == 2) {
			float clearColor[] = { 0.f,0.f,0.f,1.f };
			_cmdList->ClearRenderTargetView(handles[i], clearColor, 0, nullptr);
			continue;
		 }
		_cmdList->ClearRenderTargetView(handles[i], clearColor, 0, nullptr);
	}
	//深度クリア
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//ビューポート、シザー矩形のセット
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());

	//深度SRVセット
	//ライト始点の深度を
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	auto handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	//ライトからの深度を使用するため2つ目のビューを使う
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(3, handle);
}

//1パス目の描画
void Dx12Wrapper::DrawRenderTarget1(std::shared_ptr<PMDRenderer>& renderer)
{
	//描画処理
}

//1パス目の描画終了
void Dx12Wrapper::EndDrawRenderTarget1()
{
	//バリア設定
	for (auto& pera : _peraResource)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pera.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		_cmdList->ResourceBarrier(1, &barrier);
	}
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_bloomBuffer[0].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrier);
}

//ペラポリゴン2枚目描画準備
void Dx12Wrapper::PreDrawRenderTarget2()
{
	//バリア設定
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_peraResource2.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	auto rtvHeapPointer = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	//2パス目をレンダーターゲットにする
	rtvHeapPointer.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//深度を指定
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//2パス目のレンダーターゲットセット
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);

	//画面クリア
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//白色
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	//深度クリア
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//ビューポート、シザー矩形のセット
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());

	//1枚目ペラポリゴン用の描画パイプラインに合わせる
	_cmdList->SetPipelineState(_peraPipeline.Get());
	//同じ
	_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());
	//プリミティブトポロジー設定
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//ペラポリゴン2枚目描画(X方向ガウシアンフィルタ適用)
void Dx12Wrapper::DrawRenderTarget2()
{
	//まずヒープをセット
	_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	auto handle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	//パラメーター0番とヒープを関連付ける
	//1パス目の描画結果(通常描画)をテクスチャとして送信する
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	//コンスタントバッファ―をセット
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;
	_cmdList->SetGraphicsRootDescriptorTable(1, handle);
	
	//深度をテクスチャとして利用するためのディスクリプタヒープセット
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(3, _depthSRVHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

//ペラポリゴン2枚目描画終了(X方向ガウシアンフィルタ適用)
void Dx12Wrapper::EndDrawRenderTarget2()
{
	//バリア設定
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_peraResource2.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrier);
}

//最終レンダリングの準備
void Dx12Wrapper::PreDrawFinalRenderTarget()
{
	//バックバッファのインデックスを取得
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	//バリア設定
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	//レンダーターゲットを指定
	auto rtvHeapPointer = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//レンダーターゲットセット
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//画面クリア
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//白色
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	//ビューポート、シザー矩形のセット
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());
	//最終レンダリング用のパイプラインセット
	_cmdList->SetPipelineState(_peraPipeline.Get());
	_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());
}

//最終レンダリング結果の描画
//バックバッファ―への描画
void Dx12Wrapper::DrawFinalRenderTarget()
{
	//まずヒープをセット
	//1枚目のペラポリゴンの描画結果をテクスチャとして取得
	_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	auto handle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();

	//これで前パスのレンダリング結果をテクスチャとして利用できる(シェーダーに渡される)
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	//コンスタントバッファ―のディスクリプタ紐づけ
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//エフェクト用テクスチャ
	_cmdList->SetDescriptorHeaps(1, _effectSRVHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(2, _effectSRVHeap->GetGPUDescriptorHandleForHeapStart());
	
	//深度をテクスチャとして利用するためのディスクリプタヒープセット
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(3, _depthSRVHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

void Dx12Wrapper::DrawShrinkTextureForBlur()
{
	_cmdList->SetPipelineState(_blurPipeline.Get());
	_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());

	//頂点バッファーセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);

	//縮小バッファーはレンダーターゲットに
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	auto srvHandle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	auto rtvBaseHandle = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto rtvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {};
	//4つ目に移動(ブルーム縮小バッファー)
	rtvHandles[0].InitOffsetted(rtvBaseHandle, rtvIncSize * 3);
	//5つ目に移動(被写界深度縮小バッファー)
	rtvHandles[1].InitOffsetted(rtvBaseHandle, rtvIncSize * 4);
	//レンダーターゲットセット
	_cmdList->OMSetRenderTargets(2, rtvHandles, false, nullptr);

	//テクスチャは1枚目の3つ目のレンダーターゲット(高輝度)
	//srvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;

	//1パス目の高輝度をテクスチャとして使用
	//シェーダー側でtexHighLumを使用する
	_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(0, srvHandle);

	auto desc = _bloomBuffer[0]->GetDesc();
	D3D12_VIEWPORT vp = {};
	D3D12_RECT sr = {};

	vp.MaxDepth = 1.f;
	vp.MinDepth = 0.f;
	vp.Height = desc.Height / 2;
	vp.Width = desc.Width / 2;
	sr.top = 0;
	sr.left = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	//ここでビューポート、シザー矩形を変更しているので、
	//最終レンダリング前に再設定必要あり←ハマった
	for (int i = 0; i < 8; ++i)
	{
		_cmdList->RSSetViewports(1, &vp);
		_cmdList->RSSetScissorRects(1, &sr);
		_cmdList->DrawInstanced(4, 1, 0, 0);

		//書いたら下にずらして次を書く準備
		sr.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = sr.top;

		//幅も高さも半分に
		vp.Width /= 2;
		vp.Height /= 2;
		sr.bottom = sr.top + vp.Height;
	}

	//高輝度成分バッファーはシェーダーリソースに
	//縮小バッファーをシェーダーリソースに
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrier);
}

//シャドウ描画準備
void Dx12Wrapper::PreDrawShadow()
{
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//ライト用のビューに切り替え
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//レンダーターゲットなし
	_cmdList->OMSetRenderTargets(0, nullptr, false, &handle);	//ライトからの深度セット
	//深度バッファクリア
	_cmdList->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	D3D12_VIEWPORT vp = CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_difinition, shadow_difinition);
	_cmdList->RSSetViewports(1, &vp);//ビューポート

	CD3DX12_RECT rc(0, 0, shadow_difinition, shadow_difinition);
	_cmdList->RSSetScissorRects(1, &rc);//シザー(切り抜き)矩形
}

//シャドウ描画修了
void Dx12Wrapper::EndDrawShadow()
{
}

void
Dx12Wrapper::BeginDraw() {
	//DirectX処理
	//バックバッファのインデックスを取得
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	//レンダーターゲットを指定
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto rtvHeapPointer = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	//深度を指定
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	//1パス目
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	//画面クリア
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//白色
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);

	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);	//正面からの深度セット
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//ビューポート、シザー矩形のセット
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());
}

void
Dx12Wrapper::SetScene() {
	//現在のシーン(ビュープロジェクション)をセット
	ID3D12DescriptorHeap* sceneheaps[] = { _sceneDescHeap.Get() };
	_cmdList->SetDescriptorHeaps(1, _sceneDescHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(0, _sceneDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void
Dx12Wrapper::EndDraw() {

	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	_cmdList->ResourceBarrier(1, &barrier);

	//命令のクローズ
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	////待ち
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() < _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);//再びコマンドリストをためる準備
}


ComPtr < IDXGISwapChain4>
Dx12Wrapper::Swapchain() {
	return _swapchain;
}