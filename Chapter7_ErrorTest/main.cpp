#include <windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <d3dx12.h>	//DirectXTexに入っているものを使用
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace std;
using namespace DirectX;

//PMD頂点構造体
struct PMDVertex
{
	XMFLOAT3 pos;				//頂点座標:12バイト
	XMFLOAT3 normal;			//法線ベクトル:12バイト
	XMFLOAT2 uv;				//uv座標:8バイト
	unsigned short boneNo[2];	//ボーン番号（後述）:4バイト
	unsigned char boneWeight;	//ボーン影響度（後述）:1バイト
	unsigned char edgeFlg;		//輪郭線フラグ:1バイト
};//合計38バイト

constexpr size_t pmdvertex_size = 38;//頂点１つあたりのサイズ

// @brief コンソール画面にフォーマット付き文字列を表示
// @param formatフォーマット(%dとか%fとかの）
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

// 面倒だけど書かなければいけない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// ウィンドウが破棄された呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);// OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);// 規定の処理を行う
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* dev_ = nullptr;
IDXGIFactory6* dxgiFactory_ = nullptr;
IDXGISwapChain4* swapchain_ = nullptr;
ID3D12CommandAllocator* cmdAllocator_ = nullptr;
ID3D12GraphicsCommandList* cmdList_ = nullptr;
ID3D12CommandQueue* cmdQueue_ = nullptr;

void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	DebugOutputFormatString("Shor window test.");
	getchar();
	return 0;
#endif // _DEBUG

	// ウィンドウを作成して表示する
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;// コールバック関数の指定
	w.lpszClassName = _T("DX12 行列変換");// アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);// ハンドルの取得

	RegisterClassEx(&w); // アプリケーションクラス(ウィンドウクラスの指定をOSに伝える)

	RECT wrc = { 0,0,window_width, window_height }; // ウィンドウサイズを決める
	// 関数を使ってウィンドウサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクト生成
	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12 PMDモデル表示"),// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	// タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,			// 表示X座標はOSにお任せします
		CW_USEDEFAULT,			// 表示Y座標はOSにお任せ
		wrc.right - wrc.left,	// ウィンドウ幅
		wrc.bottom - wrc.top,	// ウィンドウ高
		nullptr,				// 親ウィンドウハンドル
		nullptr,				// メニューハンドル
		w.hInstance,			// 呼び出しアプリケーションハンドル
		nullptr);				// 追加パラメータ

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);






#ifdef _DEBUG
	// デバッグレイヤーをオンに
	// デバイス生成前にやっておかないと、デバイス生成後にやると
	// デバイスがロストしてしまうので注意
	EnableDebugLayer();
#endif // _DEBUG





	/*
	* DirectX初期化
	*/
	// FEATURE_LEVEL列挙
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	HRESULT result = S_OK;

	/* D3D12CreateDeviceに渡すアダプターを探す */
#ifdef _DEBUG
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory_));
#else
	// DXGIFactoryオブジェクトの生成
	CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory_));
#endif // _DEBUG

	// 利用可能なアダプターの列挙用
	std::vector<IDXGIAdapter*> adapters;

	// ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;

	// 利用可能なアダプターを列挙
	for (int i = 0;
		dxgiFactory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		adapters.push_back(tmpAdapter);
	}

	// 利用可能なアダプターリストから欲しいアダプターを探してセット
	for (auto adpt : adapters) {
		// アダプターを識別するための情報(構造体)
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	// アダプターの説明オブジェクト取得

		std::wstring strDesc = adesc.Description;

		// 探したいアダプターの名前を確認
		// 欲しいアダプターが見つかったらセット
		// 今回は名前にNVIDIAが含まれているアダプターを探す
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels) {
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&dev_)) == S_OK) {
			featureLevel = lv;
			break;	// 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	// コマンドリストアロケーター生成
	result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&cmdAllocator_));
	// コマンドリスト生成
	result = dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator_, nullptr, IID_PPV_ARGS(&cmdList_));

	/* コマンドキュー */
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	// タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//　アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.NodeMask = 0;
	// プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// キュー生成 
	result = dev_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue_));

	/*
	* スワップチェーン生成
	*/
	// スワップチェーンの設定情報の構造体
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	//swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;

	// バックバッファ―は伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	// フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// ウィンドウ⇔フルスクリーン切り替え
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// スワップチェーン生成
	result = dxgiFactory_->CreateSwapChainForHwnd(
		cmdQueue_,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapchain_);

	/* ディスクリプタヒープ(レンダーターゲットビュー)を作成する */

	// ディスクリプタヒープの設定情報の構造体
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;	// 裏表の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 特に指定なし

	ID3D12DescriptorHeap* rtvHeaps = nullptr;

	result = dev_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	/* ディスクリプタとスワップチェーン上のバッファ―とを関連付ける(メモリ領域の関連付け) */
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	// スワップチェーンの設定情報を取得する
	result = swapchain_->GetDesc(&swcDesc);

	// ディスクリプタヒープの先頭アドレス取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();


	//SRGBレンダーターゲットビューせてい
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//ガンマ補正あり(sRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (size_t idx = 0; idx < swcDesc.BufferCount; idx++) {
		// スワップチェーンの中のメモリ取得
		result = swapchain_->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		// レンダーターゲットビュー生成
		// 第3引数は、アドレスではなくハンドルなので、ポインタの単純なインクリメントは使用できない
		//dev_->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
		dev_->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
		// レンダーターゲットビューのサイズ分ポインタをずらす
		// 次のディスクリプタハンドルを取得できるようにする
		handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	/* フェンス生成 */
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = dev_->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));





	/*
	* PMD描画に必要な処理
	*/
	//PMDヘッダ構造体
	struct PMDHeader {
		float version;		//例:00 00 80 3F == 1.00
		char model_name[20];//モデル名
		char comment[256];	//モデルコメント
	};

	PMDHeader pmdheader;
	char signature[3] = {};	//シグネチャ
	FILE* fp = nullptr;
	errno_t error = fopen_s(&fp, "Model/初音ミク.pmd", "rb");
	if (error != 0) return 0;

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);
	//頂点数はヘッダーデータの直後
	unsigned int vertNum;//頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);

	//バッファ―確保
	//サイズ:頂点数 * １頂点のサイズ
	//std::vector<PMDVertex> vertices(vertNum);
	//for (auto i = 0; i < vertNum; i++) {
	//	fread(&vertices[i], pmdvertex_size, 1, fp);
	//}
	
	std::vector<unsigned char> vertices(vertNum* pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	fclose(fp);


	/* 頂点バッファ―作成 */
	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMDVertex));
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size());
	ID3D12Resource* vertBuff = nullptr;
	result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	//PMDVertexのサイズがアライメントで40になっていたため、
	//stribe = 38との不整合が生じた？
	/* 頂点情報のコピー */
	//PMDVertex* vertMap = nullptr;
	unsigned char* vertMap = nullptr;

	result = vertBuff->Map(0, nullptr, (void**)&vertMap);

	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);	// もうマップを解除してよい

/* 頂点バッファ―ビューの作成 */
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// バッファの仮想アドレス
	//vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMDVertex));	// 全バイト数
	vbView.SizeInBytes = vertices.size();	// 全バイト数
	vbView.StrideInBytes = pmdvertex_size;	// 1頂点あたりのバイト数

	/* シェーダーの読み込みと生成 */
	// シェーダーオブジェクト
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;

	// 頂点シェーダー読み込み
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",// シェーダー名
		nullptr,	// defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicVS", "vs_5_0",	// 関数はBasicVS、対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用及び最適化なし
		0,
		&_vsBlob, &errorBlob);	// エラー時はerrorBlobにメッセージが入る

	// シェーダー読み込みのエラーチェック
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;	// エラーメッセージ受け取り用string
			errstr.resize(errorBlob->GetBufferSize()); // 必要なサイズ分確保
			// データコピー
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";

			::OutputDebugStringA(errstr.c_str());
		}
		exit(1); // 行儀悪いかな
	}

	// ピクセルシェーダー読み込み
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",// シェーダー名
		nullptr,	// defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicPS", "ps_5_0",	// 関数はBasicVS、対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用及び最適化なし
		0,
		&_psBlob, &errorBlob);	// エラー時はerrorBlobにメッセージが入る

	// シェーダー読み込みのエラーチェック
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;	// エラーメッセージ受け取り用string
			errstr.resize(errorBlob->GetBufferSize()); // 必要なサイズ分確保
			// データコピー
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";

			::OutputDebugStringA(errstr.c_str());
		}
		exit(1); // 行儀悪いかな
	}

	/* 頂点レイアウト */
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	//12
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{	//12
			"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{	//8	
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		//以下のセマンティクスはユーザー定義
		{	//4
			"BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{	//1
			"WEIGHT",0,DXGI_FORMAT_R8_UINT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0,
		},
		{	//1
			"EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
	};

	/* グラフィックスパイプライン */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr; // 後で設定する

	// シェーダーのセット
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	// サンプルマスク、ラスタライザーステートの設定
	// デフォルトのサンプルマスク
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;	// 中身は0xffffffff
	// まだアンチエイリアスは使わないためfalse
	gpipeline.RasterizerState.MultisampleEnable = false;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	// 中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;	// 深度方向のクリッピングは有効に

	// ブレンドステートの設定
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// 入力レイアウトの設定
	gpipeline.InputLayout.pInputElementDescs = inputLayout;	// レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);	// レイアウト配列の要素数
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // ストリップ時のカット無し
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形で構成
	// レンダーターゲットの設定
	gpipeline.NumRenderTargets = 1; // 今は1つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0〜1に正規化されたRGBA
	// アンチエイリアシングのためのサンプル数設定
	gpipeline.SampleDesc.Count = 1;	// サンプリングは1ピクセルにつき1
	gpipeline.SampleDesc.Quality = 0;	// クオリティは最低

/* ルートシグネチャの作成 */

	////ディスクリプタレンジ
	//D3D12_DESCRIPTOR_RANGE descTblRange;

	//descTblRange.NumDescriptors = 1;//定数1つ
	//descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//種別は定数
	//descTblRange.BaseShaderRegister = 0;//0番スロットから
	//descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ルートパラメーター定義(ディスクリプタテーブルの設定)
	//ディスクリプタテーブルは、CPUとGPUがやり取りするリソース(テクスチャや定数バッファー)などの
	//レジスター種類、レジスター番号のセットをまとめておくもの
	//ルートシグネチャ作成の際に設定するもの(やりとりするリソースがないのなら設定しなくてもよい)
	//D3D12_ROOT_PARAMETER rootparam = {};

	//rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	////配列先頭アドレス
	//rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
	////ディスクリプタレンジ数
	//rootparam.DescriptorTable.NumDescriptorRanges = 1;
	////全てのシェーダーから見える
	//rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	////サンプラーの設定
	////サンプラー＝テクスチャマッピングの設定
	////これもルートシグネチャに設定する
	//D3D12_STATIC_SAMPLER_DESC samplerDesc = {};

	//samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横方向の繰り返し
	//samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦方向の繰り返し
	//samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行きの繰り返し
	//samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;//ボーダーは黒
	//samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//線形補間
	//samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	//samplerDesc.MinLOD = 0.f;//ミップマップ最小値
	//samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダーから見える
	//samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//リサンプリングしない

	//ルートシグネチャ作成
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	// 頂点情報がある
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//rootSignatureDesc.pParameters = &rootparam;
	//rootSignatureDesc.NumParameters = 1;

	// バイナリコード作成
	ID3DBlob* rootSigBlob = nullptr;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,	// ルートシグネチャバージョン
		&rootSigBlob,	// シェーダーを作った時と同じ
		&errorBlob);	// エラー処理も同じ

	// ルートシグネチャオブジェクトの作成
	result = dev_->CreateRootSignature(
		0,	// nodemask 0でよい
		rootSigBlob->GetBufferPointer(),	// シェーダーのときと同様
		rootSigBlob->GetBufferSize(),		// シェーダーのときと同様
		IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	// グラフィックスパイプラインステートオブジェクトの生成
	ID3D12PipelineState* _pipelineState = nullptr;
	result = dev_->CreateGraphicsPipelineState(
		&gpipeline, IID_PPV_ARGS(&_pipelineState));

	/* ビューポートとシザー矩形 */

	// ビューポート
	D3D12_VIEWPORT viewport = {};

	viewport.Width = window_width;		// 出力先の幅(ピクセル数)
	viewport.Height = window_height;	// 出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0;	// 出力先の左上座標X
	viewport.TopLeftY = 0;	// 出力先の左上座標Y
	viewport.MaxDepth = 1.0f;	// 深度最大値
	viewport.MinDepth = 0.f;	// 深度最小値

	// シザー矩形
	D3D12_RECT scissorrect = {};

	scissorrect.top = 0;	// 切り抜き上座標
	scissorrect.left = 0;	// 切り抜き下座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標


	auto worldMat = XMMatrixIdentity();

	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	// ビュー行列
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	// プロジェクション行列
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,//画角は90°
		static_cast<float>(window_width) / static_cast<float>(window_height),//アスペクト比
		1.f,//近いほう
		100.f//遠いほう
	);

	//matrix.r[0].m128_f32[0] = 2.f / window_width;	//1行1列
	//matrix.r[1].m128_f32[1] = -2.f / window_height; //2行2列
	//matrix.r[3].m128_f32[0] = -1.f;
	//matrix.r[3].m128_f32[1] = 1.f;

	////定数バッファ作成
	//heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//resdesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff);
	//ID3D12Resource* constBuff = nullptr;
	//dev_->CreateCommittedResource(
	//	&heapprop,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resdesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(&constBuff)
	//);
	////マップ
	//XMMATRIX* mapMatrix;//マップ先を示すポインター
	//result = constBuff->Map(0, nullptr, (void**)&mapMatrix);//マップ
	//*mapMatrix = worldMat * viewMat * projMat;//行列の内容をコピー
	////テクスチャ用のディスクリプタヒープ作成
	//ID3D12DescriptorHeap* basicDescHeap = {};
	//D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	////シェーダーから見えるように
	//descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	////マスクは0
	//descHeapDesc.NodeMask = 0;
	////CBV１つ
	//descHeapDesc.NumDescriptors = 1;

	////生成
	//result = dev_->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));

	///*定数バッファ―ビューの生成*/
	//auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	//cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);
	////定数バッファ―ビューの生成
	//dev_->CreateConstantBufferView(&cbvDesc, basicHeapHandle);








	/* メインループ */
	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// アプリケーションが終わる時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}

		//*mapMatrix = worldMat * viewMat * projMat;

		// DirectX処理
		// コマンドアロケーターの命令リストをリセット
		//result = cmdAllocator_->Reset();
		// 現在のバッファ―インデックス取得
		// 次のフレームで描画されるバックバッファのインデックス
		// すなわち、裏画面
		auto bbIdx = swapchain_->GetCurrentBackBufferIndex();
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdList_->ResourceBarrier(1, &barrier);

		// レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cmdList_->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// 画面クリア
		float clearColor[] = { 1.f, 1.f, 1.f, 1.f };	// 黄色
		//レンダーターゲットビュー初期化
		cmdList_->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		//パイプライン設定
		cmdList_->SetPipelineState(_pipelineState);
		cmdList_->SetGraphicsRootSignature(rootsignature);
		//ビューポート、シザー矩形設定
		cmdList_->RSSetViewports(1, &viewport);
		cmdList_->RSSetScissorRects(1, &scissorrect);

		cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		//cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		cmdList_->IASetVertexBuffers(0, 1, &vbView);

		cmdList_->DrawInstanced(vertNum, 1, 0, 0);
		// 前後の状態入れ替え
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		cmdList_->ResourceBarrier(1, &barrier);

		// 命令のクローズ
		// 命令の受付を辞める
		cmdList_->Close();





		// コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { cmdList_ };
		cmdQueue_->ExecuteCommandLists(1, cmdlists);
		// 待ち
		cmdQueue_->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			// イベントが発生するまで待ち続ける(INFINITE)
			WaitForSingleObject(event, INFINITE);
			// イベントハンドルを閉じる
			CloseHandle(event);
		}

		cmdAllocator_->Reset();	// キューをクリア
		cmdList_->Reset(cmdAllocator_, _pipelineState); // 再びコマンドリストをためる準備(Closeの解除)

		// フリップ
		swapchain_->Present(1, 0);
	}

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
}

