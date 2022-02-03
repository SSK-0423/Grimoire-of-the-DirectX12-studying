#include "PMDRenderer.h"
#include<d3dx12.h>
#include<cassert>
#include<d3dcompiler.h>
#include"Dx12Wrapper.h"
#include<string>
#include<algorithm>

using namespace std;

namespace {
	void PrintErrorBlob(ID3DBlob* blob) {
		assert(blob);
		string err;
		err.resize(blob->GetBufferSize());
		copy_n((char*)blob->GetBufferPointer(),err.size(),err.begin());
	}
}

PMDRenderer::PMDRenderer(Dx12Wrapper& dx12):_dx12(dx12)
{
	assert(SUCCEEDED(CreateRootSignature()));
	assert(SUCCEEDED(CreateGraphicsPipelineForPMD()));
	_whiteTex= CreateWhiteTexture();
	_blackTex = CreateBlackTexture();
	_gradTex = CreateGrayGradationTexture();
}


PMDRenderer::~PMDRenderer()
{
}


void 
PMDRenderer::Update() {

}
void 
PMDRenderer::Draw() {

}

//ライト視点描画準備
void PMDRenderer::BeforeDrawFromLight()
{
	_dx12.CommandList()->SetPipelineState(_shadowPipeline.Get());
	_dx12.CommandList()->SetGraphicsRootSignature(_rootSignature.Get());
	_dx12.CommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//通常描画準備
void PMDRenderer::BeforeDraw()
{
	_dx12.CommandList()->SetPipelineState(_pipeline.Get());
	_dx12.CommandList()->SetGraphicsRootSignature(_rootSignature.Get());
}

ID3D12Resource* 
PMDRenderer::CreateDefaultTexture(size_t width, size_t height) {
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	ID3D12Resource* buff = nullptr;
	auto result = _dx12.Device()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&buff)
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return nullptr;
	}
	return buff;
}

ID3D12Resource* 
PMDRenderer::CreateWhiteTexture() {
	
	ID3D12Resource* whiteBuff = CreateDefaultTexture(4,4);
	
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	auto result = whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return whiteBuff;
}
ID3D12Resource*	
PMDRenderer::CreateBlackTexture() {

	ID3D12Resource* blackBuff = CreateDefaultTexture(4, 4);
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	auto result = blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());
	assert(SUCCEEDED(result));
	return blackBuff;
}
ID3D12Resource*	
PMDRenderer::CreateGrayGradationTexture() {
	ID3D12Resource* gradBuff = CreateDefaultTexture(4, 256);
	//上が白くて下が黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);//RGBAが逆並びしているためRGBマクロと0xff<<24を用いて表す。
		std::fill(it, it + 4, col);
		--c;
	}

	auto result = gradBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(unsigned int), sizeof(unsigned int)*data.size());
	assert(SUCCEEDED(result));
	return gradBuff;
}

bool 
PMDRenderer::CheckShaderCompileResult(HRESULT result, ID3DBlob* error) {
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(error->GetBufferSize());
			std::copy_n((char*)error->GetBufferPointer(), error->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		return false;
	}
	else {
		return true;
	}
}

//パイプライン初期化
HRESULT 
PMDRenderer::CreateGraphicsPipelineForPMD() {
	ComPtr<ID3DBlob> vsBlob = nullptr;
	ComPtr<ID3DBlob> psBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	auto result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vsBlob, &errorBlob);
	if (!CheckShaderCompileResult(result,errorBlob.Get())){
		assert(0);
		return result;
	}
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &psBlob, &errorBlob);
	if (!CheckShaderCompileResult(result, errorBlob.Get())) {
		assert(0);
		return result;
	}
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONENO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{"SV_InstanceID",0,DXGI_FORMAT_R32_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		//{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = _rootSignature.Get();
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff


	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない

	gpipeline.DepthStencilState.DepthEnable = true;//深度バッファを使うぞ
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	//マルチレンダーターゲットであることを指定する
	gpipeline.NumRenderTargets = 3;//通常カラー、法線、高輝度
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA
	gpipeline.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA
	gpipeline.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低
	result = _dx12.Device()->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipeline.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
	}

	//影用のパイプライン生成
	result = D3DCompileFromFile(L"ShadowVS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"shadowVS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), errorBlob.ReleaseAndGetAddressOf());
	if (!CheckShaderCompileResult(result, errorBlob.Get())) {
		assert(0);
		return result;
	}
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS.BytecodeLength = 0;
	gpipeline.PS.pShaderBytecode = nullptr;
	gpipeline.NumRenderTargets = 0;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	gpipeline.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;	//一度セットしたレンダーターゲット全てのフォーマットを変える必要がある
	gpipeline.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;	//一度セットしたレンダーターゲット全てのフォーマットを変える必要がある
	
	result = _dx12.Device()->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_shadowPipeline.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
	}

	return result;
}
//ルートシグネチャ初期化
HRESULT 
PMDRenderer::CreateRootSignature() {
	//レンジ
	CD3DX12_DESCRIPTOR_RANGE  descTblRanges[5] = {};//テクスチャと定数の２つ
	descTblRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);//定数[b0](ビュープロジェクション用)
	descTblRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);//定数[b1](ワールド、ボーン用)
	descTblRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);//定数[b2](マテリアル用)
	descTblRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);//テクスチャ４つ(基本とsphとspaとトゥーン) t0～t3
	descTblRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);//深度バッファテクスチャ t4

	//ルートパラメータ
	CD3DX12_ROOT_PARAMETER rootParams[4] = {};
	rootParams[0].InitAsDescriptorTable(1, &descTblRanges[0]);//ビュープロジェクション変換
	rootParams[1].InitAsDescriptorTable(1, &descTblRanges[1]);//ワールド・ボーン変換
	rootParams[2].InitAsDescriptorTable(2, &descTblRanges[2]);//マテリアル周り b2 + t0～t3
	rootParams[3].InitAsDescriptorTable(1, &descTblRanges[4]);//深度 t4

	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[3] = {};
	samplerDescs[0].Init(0);
	samplerDescs[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	
	samplerDescs[2] = samplerDescs[0];
	samplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	// <=であればtrue(1.0)そうでなければ(0.0)
	samplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDescs[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;// 比較結果をバイリニア補間
	samplerDescs[2].MaxAnisotropy = 1;	//深度傾斜を有効に
	samplerDescs[2].ShaderRegister = 2;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(_countof(rootParams), rootParams, _countof(samplerDescs), samplerDescs, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	auto result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSigBlob, &errorBlob);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	result = _dx12.Device()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(_rootSignature.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	return result;
}

ID3D12PipelineState* 
PMDRenderer::GetPipelineState() {
	return _pipeline.Get();
}

ID3D12RootSignature* 
PMDRenderer::GetRootSignature() {
	return _rootSignature.Get();
}