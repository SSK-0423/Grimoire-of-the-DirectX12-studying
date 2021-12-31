//コンスタントバッファで行列を転送
#include "Application.h"
#include "RefacApplication.h"

using namespace DirectX;
using namespace Microsoft::WRL;

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void Application::DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

////面倒だけど書かなあかんやつ
//LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
//	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
//		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
//		return 0;
//	}
//	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
//}


HRESULT Application::EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();

	return result;
}

// モデルのパスとテクスチャのパスから合成パスを得る
// @param modelPath アプリケーションから見たpmdモデルのパス
// @param texPath PMDモデルから見たテクスチャのパス
// @param アプリケーションから見たテクスチャのパス
std::string Application::GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath
)
{
	auto folderPath = modelPath.substr(0, modelPath.rfind('/') + 1);
	return folderPath + texPath;
}

// ファイル名から拡張子を取得する
// @param path 対象のパス文字列
// @return 拡張子
std::string Application::GetExtension(const std::string& path) {
	size_t idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

// テクスチャのパスをセパレータ文字で分離する
// @param path 対象のパス文字列
// @param spliter 区切り文字
// @return 分離前後の文字列ペア
std::pair<std::string, std::string> Application::SplitFileName(
	const std::string& path, const char splitter)
{
	size_t idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

// std::string （マルチバイト文字列）からstd::wstring（ワイド文字列）を得る
// @param str マルチバイト文字列
// @param 変換されたワイド文字列
std::wstring Application::GetWideStringFromString(const std::string& str) {
	//呼び出し1回目(文字列数を得る)
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0
	);

	std::wstring wstr;	// stringのwchar_t版
	wstr.resize(num1);	// 得られた文字列数でリサイズ

	// 呼び出し2回目（確保済みのwstrに変換文字列をコピー）
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1
	);

	assert(num1 == num2);	//一応チェック
	return wstr;
}

//テクスチャファイルの読み込み
ComPtr<ID3D12Resource> Application::LoadTextureFromFile(std::string& texPath)
{
	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end()) {
		//テーブル内にあったらロードするのではなく
		//マップ内のリソースを返す
		return _resourceTable[texPath];
	}

	// WICテクスチャロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	auto wtexpath = GetWideStringFromString(texPath);	//テクスチャファイルのパス
	auto ext = GetExtension(texPath);	//拡張子を取得
	auto result = LoadLambdaTable[ext](wtexpath, &metadata, scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0);	//生データ抽出
	// WriteToSubresourceで転送する用のヒープ設定
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	//バッファ―作成
	ComPtr<ID3D12Resource> texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,	// 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texbuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		return nullptr;
	}

	result = texbuff->WriteToSubresource(
		0,
		nullptr,		//全領域へコピー
		img->pixels,	//元データアドレス
		img->rowPitch,	//1ラインサイズ
		img->slicePitch	//全サイズ
	);

	if (FAILED(result)) {
		return nullptr;
	}

	_resourceTable[texPath] = texbuff;
	return texbuff;
}

ComPtr<ID3D12Resource> Application::CreateWhiteTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> whiteBuff = nullptr;

	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	//データ転送
	result = whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<UINT>(data.size()));
	return whiteBuff;
}

ComPtr<ID3D12Resource> Application::CreateBlackTexture() {
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> blackBuff = nullptr;

	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(blackBuff.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	//データ転送
	result = blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<UINT>(data.size()));
	return blackBuff;
}

ComPtr<ID3D12Resource> Application::CreateGrayGradationTexture() {
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	ComPtr<ID3D12Resource> gradBuff = nullptr;
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		return nullptr;
	}

	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + 4, col);//データ埋め
		--c;
	}

	result = gradBuff->WriteToSubresource(0, nullptr, data.data(),
		4 * sizeof(unsigned int), sizeof(unsigned) * static_cast<UINT>(data.size()));
	return gradBuff;
}
//ゲームウィンドウ作成
void Application::CreateGameWindow()
{
	////以下を書いておかないとCOMが旨く動かずWICが正常に動作しないことがあります。
	////(書かなくても動くときもあります)
	//result = CoInitializeEx(0, COINIT_MULTITHREADED);

	//DebugOutputFormatString("Show window test.");
	//HINSTANCE hInst = GetModuleHandle(nullptr);
	////ウィンドウクラス生成＆登録
	//windowClass = {};
	//windowClass.cbSize = sizeof(WNDCLASSEX);
	//windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	//windowClass.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	//windowClass.hInstance = GetModuleHandle(0);//ハンドルの取得
	//RegisterClassEx(&windowClass);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	//RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
	//AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	////ウィンドウオブジェクトの生成
	//hwnd = CreateWindow(windowClass.lpszClassName,//クラス名指定
	//	_T("DX12テスト PMDモデル表示"),//タイトルバーの文字
	//	WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
	//	CW_USEDEFAULT,//表示X座標はOSにお任せします
	//	CW_USEDEFAULT,//表示Y座標はOSにお任せします
	//	wrc.right - wrc.left,//ウィンドウ幅
	//	wrc.bottom - wrc.top,//ウィンドウ高
	//	nullptr,//親ウィンドウハンドル
	//	nullptr,//メニューハンドル
	//	windowClass.hInstance,//呼び出しアプリケーションハンドル
	//	nullptr);//追加パラメータ
}

//デバイスとDXGI生成
HRESULT Application::CreateDeviceAndDXGIFactory()
{
	//DirectX12まわり初期化
	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

	RETURN_FAILED(result);

	//アダプター列挙
	std::vector <ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}

	//"NVIDIA"のGPUを探す
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (SUCCEEDED(D3D12CreateDevice(tmpAdapter.Get(), l, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())))) {
			featureLevel = l;
			result = S_OK;
			break;
		}
	}

	return result;
}

//コマンドアロケータ、リスト、キュー生成
HRESULT Application::CreateCommandX()
{
	//コマンドアロケータ生成
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);
	//コマンドリスト生成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	//コマンドキュー生成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//プライオリティ特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//ここはコマンドリストと合わせてください
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));//コマンドキュー生成

	return result;
}

//スワップチェーン生成
HRESULT Application::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue.Get(),
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf());

	return result;
}

HRESULT Application::CreateFinalRenderTarget()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeaps.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	RETURN_FAILED(result);

	_backBuffers.resize(swcDesc.BufferCount);
	rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(_backBuffers[i].ReleaseAndGetAddressOf()));
		RETURN_FAILED(result);
		rtvDesc.Format = _backBuffers[i]->GetDesc().Format;
		_dev->CreateRenderTargetView(_backBuffers[i].Get(), &rtvDesc, rtvH);
		rtvH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	viewport = CD3DX12_VIEWPORT(_backBuffers[0].Get());
	scissorrect = CD3DX12_RECT(0, 0, window_width, window_height);

	return result;
}

//デプスステンシルビュー生成
HRESULT Application::CreateDepthStencilView()
{
	//深度バッファ作成
	//深度バッファの仕様
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元のテクスチャデータとして
	depthResDesc.Width = window_width;//幅と高さはレンダーターゲットと同じ
	depthResDesc.Height = window_height;//上に同じ
	depthResDesc.DepthOrArraySize = 1;//テクスチャ配列でもないし3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1;//サンプルは1ピクセル当たり1つ
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//このバッファは深度ステンシルとして使用します
	depthResDesc.MipLevels = 1;
	depthResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResDesc.Alignment = 0;

	//デプス用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//DEFAULTだから後はUNKNOWNでよし
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//このクリアバリューが重要な意味を持つ
	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth = 1.0f;//深さ１(最大値)でクリア
	_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//32bit深度値としてクリア

	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //デプス書き込みに使用
		&_depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	//深度のためのデスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使うよという事がわかればいい
	dsvHeapDesc.NumDescriptors = 1;//深度ビュー1つのみ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//デプスステンシルビューとして使う

	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//デプス値に32bit使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//フラグは特になし
	_dev->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

//フェンス生成
HRESULT Application::CreateID3D12Fence()
{
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));
	return result;
}

//テクスチャ読み込みのラムダ式テーブル作成
void Application::CreateTextureLoaderTable()
{
	LoadLambdaTable["sph"] = LoadLambdaTable["spa"] = LoadLambdaTable["bmp"] = LoadLambdaTable["png"] = LoadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	LoadLambdaTable["tga"] = [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	LoadLambdaTable["dds"] = [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

//PMDファイル読み込み
HRESULT Application::LoadPMDFile(const char* path)
{
	//ヘッダー情報読込
	char signature[3];
	PMDHeader pmdheader = {};
	FILE* fp;
	auto err = fopen_s(&fp, path, "rb");
	if (fp == nullptr) {
		char strerr[256];
		strerror_s(strerr, 256, err);
		//MessageBox(hwnd, strerr,L"Open Error",MB_ICONERROR);
		return -1;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum;//頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

	constexpr unsigned int pmdvertex_size = 38;//頂点1つあたりのサイズ
	std::vector<PMD_VERTEX> vertices(vertNum);//バッファ確保
	for (auto i = 0; i < vertNum; i++)
	{
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));
	//UPLOAD(確保は可能)
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	PMD_VERTEX* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	vertBuff->Unmap(0, nullptr);

	//頂点バッファビュー作成
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));//全バイト数
	vbView.StrideInBytes = sizeof(PMD_VERTEX);//1頂点あたりのバイト数

	//インデックス
	unsigned int indicesNum;//インデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short> indices(indicesNum);

	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
	//OKだと思います。
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	//作ったバッファにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//インデックスバッファビューを作成
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	//マテリアルデータ
	fread(&materialNum, sizeof(materialNum), 1, fp);

	std::vector<PMDMaterial> pmdMaterials(materialNum);

	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp); // 一気に読み込む

	fclose(fp);

	materials.resize(pmdMaterials.size());
	//シェーダーに渡すようのリストにコピー
	for (size_t i = 0; i < pmdMaterials.size(); i++) {
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}

	//あってるかな？？？？
	textureResources.resize(materialNum);
	sphResources.resize(materialNum);
	spaResources.resize(materialNum);
	toonResources.resize(materialNum);

	for (size_t i = 0; i < pmdMaterials.size(); i++) {
		//トゥーンリソースの読み込み
		std::string toonFilePath = "toon/";

		char toonFileName[16];

		sprintf_s(toonFileName, sizeof(toonFileName), "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);

		toonFilePath += toonFileName;

		toonResources[i] = LoadTextureFromFile(toonFilePath);

		sphResources[i] == nullptr;

		if (strlen(pmdMaterials[i].texFilePath) == 0) {
			textureResources[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{	//スプリッタがある

			auto namepair = SplitFileName(texFileName);

			//スフィアマップ出ない方をテクスチャパスにする
			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa") {
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else
			{
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph") {
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa") {
					spaFileName = namepair.second;
				}
			}
		}
		else
		{
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph") {
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa") {
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
		}

		//モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
		if (texFileName != "") {
			auto texFilePath = GetTexturePathFromModelAndTexPath(path, texFileName.c_str());
			textureResources[i] = LoadTextureFromFile(texFilePath);
		}
		if (sphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(path, sphFileName.c_str());
			sphResources[i] = LoadTextureFromFile(sphFilePath);
		}
		if (spaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(path, texFileName.c_str());
			spaResources[i] = LoadTextureFromFile(spaFilePath);
		}

	}

	return S_OK;
}

//マテリアルデータ作成
HRESULT Application::CreateMaterialData()
{
	//マテリアルバッファ―作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum);
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf())
	);
	RETURN_FAILED(result);

	//マップマテリアルにコピー
	char* mapMaterial = nullptr;
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);
	RETURN_FAILED(result);

	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;//データコピー
		mapMaterial += materialBuffSize;	//次のアライメント位置まで進める（256の倍数）
	}
	materialBuff->Unmap(0, nullptr);

	//マテリアル用ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum * 5;
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf()));
	RETURN_FAILED(result);

	//マテリアル用ビュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;

	//通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	//デフォルト
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;	// ミップマップは使用しないので1

	//先頭を記録
	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//白テクスチャ
	whiteTex = CreateWhiteTexture();
	//黒テクスチャ
	blackTex = CreateBlackTexture();
	//グラデーションテクスチャ
	gradTex = CreateGrayGradationTexture();

	for (size_t i = 0; i < materialNum; i++) {
		//マテリアル用定数バッファ―ビュー
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);

		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += materialBuffSize;

		//シェーダーリソースビュー（ここからが追加部分）
		if (textureResources[i] == nullptr) {
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = textureResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(textureResources[i].Get(), &srvDesc, matDescHeapH);
		}

		//乗算スフィアマップ
		matDescHeapH.ptr += inc;
		if (sphResources[i] == nullptr) {
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(sphResources[i].Get(), &srvDesc, matDescHeapH);
		}

		//加算スフィアマップ
		matDescHeapH.ptr += inc;
		if (spaResources[i] == nullptr) {
			srvDesc.Format = blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(spaResources[i].Get(), &srvDesc, matDescHeapH);
		}

		//トゥーンテクスチャ
		matDescHeapH.ptr += inc;
		if (toonResources[i] == nullptr) {
			srvDesc.Format = gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(gradTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(toonResources[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += inc;
	}

	return S_OK;
}

//シェーダーオブジェクト生成
HRESULT Application::CreateShaderObject()
{
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}

	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}

	return result;
}

//ルートシグネチャ生成
HRESULT Application::CreateRootSig()
{
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	CD3DX12_DESCRIPTOR_RANGE descTblRange[3] = {};//テクスチャと定数の２つ
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);// テクスチャ4つ(基本とsphとspaとトゥーン)

	CD3DX12_ROOT_PARAMETER rootparam[2] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);//座標変換
	rootparam[1].InitAsDescriptorTable(2, &descTblRange[1]);//マテリアル周り

	rootSignatureDesc.pParameters = &rootparam[0];//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 2;//ルートパラメータ数

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	rootSignatureDesc.pStaticSamplers = &samplerDesc[0];
	rootSignatureDesc.NumStaticSamplers = 2;

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, rootSigBlob.ReleaseAndGetAddressOf(), errorBlob.ReleaseAndGetAddressOf());
	RETURN_FAILED(result);

	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootsignature.ReleaseAndGetAddressOf()));

	return result;
}

//グラフィックスパイプラインは1つのみ
HRESULT Application::CreateBasicGraphicsPipeline()
{
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

	//ハルシェーダ、ドメインシェーダ、ジオメトリシェーダは設定しない
	gpipeline.HS.BytecodeLength = 0;
	gpipeline.HS.pShaderBytecode = nullptr;
	gpipeline.DS.BytecodeLength = 0;
	gpipeline.DS.pShaderBytecode = nullptr;
	gpipeline.GS.BytecodeLength = 0;
	gpipeline.GS.pShaderBytecode = nullptr;

	//ラスタライザ(RS)
	//gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない

	//OutputMerger部分
	//レンダーターゲット
	gpipeline.NumRenderTargets = 1;//注)このターゲット数と設定するフォーマット数は
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//一致させておく事

	//深度ステンシル
	gpipeline.DepthStencilState.DepthEnable = true;//深度
	gpipeline.DepthStencilState.StencilEnable = false;//あとで
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//ブレンド設定
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.NodeMask = 0;
	gpipeline.SampleDesc.Count = 1;
	gpipeline.SampleDesc.Quality = 0;
	gpipeline.SampleMask = 0xffffffff;//全部対象
	gpipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	//ひとまず論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低


	gpipeline.pRootSignature = rootsignature.Get();
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));

	return result;
}

//座標変換用ビューの生成
HRESULT Application::CreateSceneTransformView()
{
	//定数バッファ作成
	worldMat = XMMatrixIdentity();
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2,//画角は90°
		static_cast<float>(window_width) / static_cast<float>(window_height),//アス比
		1.0f,//近い方
		100.0f//遠い方
	);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff);
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf())
	);
	RETURN_FAILED(result);

	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);//マップ
	RETURN_FAILED(result);

	//行列の内容をコピー
	mapMatrix->world = worldMat;
	mapMatrix->view = viewMat;
	mapMatrix->proj = projMat;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	descHeapDesc.NodeMask = 0;//マスクは0
	descHeapDesc.NumDescriptors = 1;//CBV1つ
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));//生成
	RETURN_FAILED(result);

	////デスクリプタの先頭ハンドルを取得しておく
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
	//定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	//constBuff->Unmap(0, nullptr);

	return result;
}

Application& Application::Instance()
{
	static Application instance;
	return instance;
}

//初期化処理
/*
* ウィンドウ生成
* デバイス生成
* スワップチェーン生成
*/
bool Application::Init()
{
	CreateGameWindow();
#ifdef _DEBUG
	//デバッグレイヤーをオンに
	if (FAILED(EnableDebugLayer())) {
		assert(0);
		return false;
	}
#endif
	//DirectX12関連初期化
	if (FAILED(CreateDeviceAndDXGIFactory())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateCommandX())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateSwapChain())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateFinalRenderTarget())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateID3D12Fence())) {
		assert(0);
		return false;
	}
	//ここまでDxWrapper
	///////////////////////////////////////

	ShowWindow(hwnd, SW_SHOW);//ウィンドウ表示

	CreateTextureLoaderTable();


	std::string strModelPath = "Model/初音ミク.pmd";
	//std::string strModelPath = "Model/初音ミクmetal.pmd";
	//std::string strModelPath = "Model/巡音ルカ.pmd";

	if (FAILED(LoadPMDFile(strModelPath.c_str()))) {
		assert(0);
		return false;
	}
	if (FAILED(CreateMaterialData())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateShaderObject())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateRootSig())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBasicGraphicsPipeline())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateSceneTransformView())) {
		assert(0);
		return false;
	}

	return true;
}

void Application::Run()
{
	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
	while (true) {
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->view = viewMat;
		mapMatrix->proj = projMat;
		angle += 0.005f;

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}
		
		///////////////////BeginDraw()///////////////////////////
		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx].Get();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		///////////////////////////////////////////////////////////

		_cmdList->SetPipelineState(_pipelinestate.Get());

		///////////////////BeginDraw()///////////////////////////
		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		_cmdList->ResourceBarrier(1, &BarrierDesc);
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
		float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };//白色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		///////////////////////////////////////////////////////////

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//////////////PMDActor::Draw/////////////////
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		////////////////////////////////////////////

		_cmdList->SetGraphicsRootSignature(rootsignature.Get());

		//////////////PMDActor::Draw////////////////
		ID3D12DescriptorHeap* bdh[] = { basicDescHeap.Get() };
		_cmdList->SetDescriptorHeaps(1, bdh);
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

		//マテリアル
		ID3D12DescriptorHeap* mdh[] = { materialDescHeap.Get() };
		_cmdList->SetDescriptorHeaps(1, mdh);

		auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart();
		unsigned int idxOffset = 0;
		auto cbvsrvIncSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5; //3倍
		for (auto& m : materials) {
			_cmdList->SetGraphicsRootDescriptorTable(1, materialH);
			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);
			//ヒープポインターとインデックスを次に進める
			materialH.ptr += cbvsrvIncSize;	//次ビューのために2倍進める　
			idxOffset += m.indicesNum;
		}
		//_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);
		/////////////////////////////////////////////

		///////////EndDraw//////////////
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//命令のクローズ
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		//待ち
		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

		while (_fence->GetCompletedValue() != _fenceVal) {
			;
		}
		///////////////////////////////

		//フリップ
		_swapchain->Present(1, 0);

		///////////EndDraw//////////////
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator.Get(), nullptr);//再びコマンドリストをためる準備
		///////////////////////////////
	}
}

void Application::Terminate()
{
	//もうクラス使わんから登録解除してや
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

Application::~Application()
{
}
