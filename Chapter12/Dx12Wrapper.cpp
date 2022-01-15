#include "Dx12Wrapper.h"
#include<cassert>
#include<d3dx12.h>
#include"Application.h"
#include "PMDRenderer.h"

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace {
	///���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	///@param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
	///@param texPath PMD���f�����猩���e�N�X�`���̃p�X
	///@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//�t�@�C���̃t�H���_��؂��\��/�̓��ނ��g�p�����\��������
		//�Ƃ�����������\��/�𓾂���΂����̂ŁA�o����rfind���Ƃ��r����
		//int�^�ɑ�����Ă���̂͌�����Ȃ������ꍇ��rfind��epos(-1��0xffffffff)��Ԃ�����
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}

	///�t�@�C��������g���q���擾����
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///�t�@�C��������g���q���擾����(���C�h������)
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	wstring
		GetExtension(const std::wstring& path) {
		int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	///@param path �Ώۂ̃p�X������
	///@param splitter ��؂蕶��
	///@return �����O��̕�����y�A
	pair<string, string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		pair<string, string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}

	///string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
	///@param str �}���`�o�C�g������
	///@return �ϊ����ꂽ���C�h������
	std::wstring
		GetWideStringFromString(const std::string& str) {
		//�Ăяo��1���(�����񐔂𓾂�)
		auto num1 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, nullptr, 0);

		std::wstring wstr;//string��wchar_t��
		wstr.resize(num1);//����ꂽ�����񐔂Ń��T�C�Y

		//�Ăяo��2���(�m�ۍς݂�wstr�ɕϊ���������R�s�[)
		auto num2 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, &wstr[0], num1);

		assert(num1 == num2);//�ꉞ�`�F�b�N
		return wstr;
	}
	///�f�o�b�O���C���[��L���ɂ���
	void EnableDebugLayer() {
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
	}
}
Dx12Wrapper::Dx12Wrapper(HWND hwnd) {
#ifdef _DEBUG
	//�f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif

	auto& app = Application::Instance();
	_winSize = app.GetWindowSize();

	//DirectX12�֘A������
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

	if (FAILED(CreatePeraResourceAndView())) {
		assert(0);
		return;
	}
	if (FAILED(CreatePeraVertexBufferAndView()))
	{
		assert(0);
		return;
	}
	if (FAILED(CreatePeraPipeline()))
	{
		assert(0);
		return;
	}
	//�e�N�X�`�����[�_�[�֘A������
	CreateTextureLoaderTable();



	//�[�x�o�b�t�@�쐬
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return;
	}

	//�t�F���X�̍쐬
	if (FAILED(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())))) {
		assert(0);
		return;
	}

}

HRESULT
Dx12Wrapper::CreateDepthStencilView() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	//�[�x�o�b�t�@�쐬
	//�[�x�o�b�t�@�̎d�l
	//auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
	//	desc.Width, desc.Height,
	//	1, 0, 1, 0,
	//	D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);


	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resdesc.DepthOrArraySize = 1;
	resdesc.Width = desc.Width;
	resdesc.Height = desc.Height;
	resdesc.Format = DXGI_FORMAT_D32_FLOAT;
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resdesc.MipLevels = 1;
	resdesc.Alignment = 0;

	//�f�v�X�p�q�[�v�v���p�e�B
	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //�f�v�X�������݂Ɏg�p
		&depthClearValue,
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		//�G���[����
		return result;
	}

	//�[�x�̂��߂̃f�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//�[�x�Ɏg����Ƃ��������킩��΂���
	dsvHeapDesc.NumDescriptors = 1;//�[�x�r���[1�̂�
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//�f�v�X�X�e���V���r���[�Ƃ��Ďg��
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;


	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));

	//�[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//�f�v�X�l��32bit�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//�t���O�͓��ɂȂ�
	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}


Dx12Wrapper::~Dx12Wrapper()
{
}

ComPtr<ID3D12Resource>
Dx12Wrapper::GetTextureByPath(const char* texpath) {
	auto it = _textureTable.find(texpath);
	if (it != _textureTable.end()) {
		//�e�[�u���ɓ��ɂ������烍�[�h����̂ł͂Ȃ��}�b�v����
		//���\�[�X��Ԃ�
		return _textureTable[texpath];
	}
	else {
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texpath));
	}

}

//�e�N�X�`�����[�_�e�[�u���̍쐬
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

//�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[
ID3D12Resource*
Dx12Wrapper::CreateTextureFromFile(const char* texpath) {
	string texPath = texpath;
	//�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = GetWideStringFromString(texPath);//�e�N�X�`���̃t�@�C���p�X
	auto ext = GetExtension(texPath);//�g���q���擾
	auto result = _loadLambdaTable[ext](wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return nullptr;
	}
	auto img = scratchImg.GetImage(0, 0, 0);//���f�[�^���o

	//WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	if (FAILED(result)) {
		return nullptr;
	}
	result = texbuff->WriteToSubresource(0,
		nullptr,//�S�̈�փR�s�[
		img->pixels,//���f�[�^�A�h���X
		img->rowPitch,//1���C���T�C�Y
		img->slicePitch//�S�T�C�Y
	);
	if (FAILED(result)) {
		return nullptr;
	}

	return texbuff;
}

HRESULT
Dx12Wrapper::InitializeDXGIDevice() {
	UINT flagsDXGI = 0;
	flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
	//DirectX12�܂�菉����
	//�t�B�[�`�����x����
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
	//Direct3D�f�o�C�X�̏�����
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

///�X���b�v�`�F�C�������֐�
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

//�R�}���h�܂�菉����
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
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�v���C�I���e�B���Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�����̓R�}���h���X�g�ƍ��킹�Ă�������
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));//�R�}���h�L���[����
	assert(SUCCEEDED(result));
	return result;
}

//�r���[�v���W�F�N�V�����p�r���[�̐���
HRESULT
Dx12Wrapper::CreateSceneView() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	//�萔�o�b�t�@�쐬
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

	_mappedSceneData = nullptr;//�}�b�v��������|�C���^
	result = _sceneConstBuff->Map(0, nullptr, (void**)&_mappedSceneData);//�}�b�v

	XMFLOAT3 eye(0, 10, -30);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��45��
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//�A�X��
		0.1f,//�߂���
		1000.0f//������
	);
	_mappedSceneData->eye = eye;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
	descHeapDesc.NodeMask = 0;//�}�X�N��0
	descHeapDesc.NumDescriptors = 1;//
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_sceneDescHeap.ReleaseAndGetAddressOf()));//����

	////�f�X�N���v�^�̐擪�n���h�����擾���Ă���
	auto heapHandle = _sceneDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _sceneConstBuff->GetDesc().Width;
	//�萔�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, heapHandle);
	return result;

}

HRESULT Dx12Wrapper::CreatePeraResourceAndView()
{
	//�g���Ă���o�b�N�o�b�t�@�\�̏��𗘗p����
	auto& bbuff = _backBuffers[0];
	D3D12_RESOURCE_DESC resDesc = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//�����_�����O���̃N���A�l�Ɠ����l
	float clsClr[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);

	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		//D3D12_RESOURCE_STATE_RENDER_TARGET�ł͂Ȃ�
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_peraResource.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//RTV�p�q�[�v�����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = _rtvHeaps->GetDesc();
	heapDesc.NumDescriptors = 1;	//�o�b�N�o�b�t�@�\�Ƃ��Ďg��Ȃ��̂�
	result = _dev->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(_peraRTVHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�����_�[�^�[�Q�b�g�r���[�����
	_dev->CreateRenderTargetView(
		_peraResource.Get(),
		&rtvDesc,
		_peraRTVHeap->GetCPUDescriptorHandleForHeapStart()
	);

	//SRV�p�q�[�v�����
	heapDesc.NumDescriptors = 1;
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

	//�V�F�[�_�[���\�[�X�r���[�����
	_dev->CreateShaderResourceView(
		_peraResource.Get(),
		&srvDesc,
		_peraSRVHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

HRESULT Dx12Wrapper::CreatePeraVertexBufferAndView()
{
	//���_�o�b�t�@�\�쐬
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

	//���_�o�b�t�@�\�r���[�쐬
	_peraVBV.BufferLocation = _peraVB->GetGPUVirtualAddress();
	_peraVBV.SizeInBytes = sizeof(pv);
	_peraVBV.StrideInBytes = sizeof(pv[0]);

	//�}�b�v
	PeraVertex* mapped = nullptr;
	_peraVB->Map(0, nullptr, (void**)&mapped);
	std::copy(std::begin(pv), std::end(pv), mapped);
	_peraVB->Unmap(0, nullptr);

	return result;
}

HRESULT Dx12Wrapper::CreatePeraPipeline()
{
	//���_���C�A�E�g�ݒ�
	D3D12_INPUT_ELEMENT_DESC layout[2] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	//�p�C�v���C���X�e�[�g�̐ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errBlob;
	//���_�V�F�[�_�[�R���p�C��
	auto result = D3DCompileFromFile(
		L"peraVertex.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs", "vs_5_0",
		0, 0, vs.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	//�s�N�Z���V�F�[�_�[�R���p�C��
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

	D3D12_DESCRIPTOR_RANGE range = {};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range.BaseShaderRegister = 0;	// 0
	range.NumDescriptors = 1;

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParam.DescriptorTable.pDescriptorRanges = &range;
	rootParam.DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);	// s0

	//���[�g�V�O�l�`������
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.NumParameters = 1;
	rsDesc.pParameters = &rootParam;
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
	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline.ReleaseAndGetAddressOf()));

	return result;
}

HRESULT
Dx12Wrapper::CreateFinalRenderTargets() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//�����_�[�^�[�Q�b�g�r���[�Ȃ̂œ��RRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//���Ɏw��Ȃ�

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		SUCCEEDED(result);
		return result;
	}
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	_backBuffers.resize(swcDesc.BufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
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

//1�p�X�ڂ̕`�揀��
void Dx12Wrapper::PreDrawRenderTarget1(std::shared_ptr<PMDRenderer>& renderer)
{
	//�o���A�ݒ�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_peraResource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	auto rtvHeapPointer = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//1�p�X�ڂ̃����_�[�^�[�Q�b�g�Z�b�g
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);

	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	//�[�x�N���A
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());
	
	//PMD�p�̕`��p�C�v���C���ɍ��킹��
	_cmdList->SetPipelineState(renderer->GetPipelineState());
	//���[�g�V�O�l�`����PMD�p�ɍ��킹��
	_cmdList->SetGraphicsRootSignature(renderer->GetRootSignature());
	//�v���~�e�B�u�g�|���W�[�ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//1�p�X�ڂ̕`��
void Dx12Wrapper::DrawRenderTarget1(std::shared_ptr<PMDRenderer>& renderer)
{
	//�`�揈��
}

//1�p�X�ڂ̕`��I��
void Dx12Wrapper::EndDrawRenderTarget1()
{
	//�o���A�ݒ�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_peraResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrier);
}

//�ŏI�����_�����O�̏���
void Dx12Wrapper::PreDrawFinalRenderTarget()
{
	//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	//�o���A�ݒ�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	//�����_�[�^�[�Q�b�g���w��
	auto rtvHeapPointer = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvHeapPointer.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//�����_�[�^�[�Q�b�g�Z�b�g
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);
	
	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	//�[�x�N���A
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	//�ŏI�����_�����O�p�̃p�C�v���C���Z�b�g
	_cmdList->SetPipelineState(_peraPipeline.Get());
	_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());

}

//�ŏI�����_�����O���ʂ̕`��
void Dx12Wrapper::DrawFinalRenderTarget()
{
	//�܂��q�[�v���Z�b�g
	_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	auto handle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	//�p�����[�^�[0�Ԃƃq�[�v���֘A�t����
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

void
Dx12Wrapper::BeginDraw() {
	////DirectX����
	////�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
	//auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	////�����_�[�^�[�Q�b�g���w��
	//auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	//rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//auto rtvHeapPointer = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	////�[�x���w��
	//auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	////1�p�X��
	//auto peraBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
	//	_backBuffers[bbIdx],
	//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//	D3D12_RESOURCE_STATE_RENDER_TARGET
	//);
	//_cmdList->ResourceBarrier(1, &peraBarrier);

	////��ʃN���A
	//float clearColor[] = { 1.0f,1.0f,1.0f,1.0f };//���F
	//_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);

	//_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);
	//_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	////�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	//_cmdList->RSSetViewports(1, _viewport.get());
	//_cmdList->RSSetScissorRects(1, _scissorrect.get());

	//peraBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
	//	_peraResource.Get(),
	//	D3D12_RESOURCE_STATE_RENDER_TARGET,
	//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	//);
	//_cmdList->ResourceBarrier(1, &peraBarrier);

	//auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
	//	D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	//_cmdList->ResourceBarrier(1, &barrier);

	//_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
	////�����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�\��
	//_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
	////�y���|���S���`��
	//_cmdList->SetPipelineState(_peraPipeline.Get());
	//_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());

	////�܂��q�[�v���Z�b�g
	//_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	//auto handle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	////�p�����[�^�[0�Ԃƃq�[�v���֘A�t����
	//_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	//_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);
	//_cmdList->DrawInstanced(4, 1, 0, 0);
}

void
Dx12Wrapper::SetScene() {
	//���݂̃V�[��(�r���[�v���W�F�N�V����)���Z�b�g
	ID3D12DescriptorHeap* sceneheaps[] = { _sceneDescHeap.Get() };
	_cmdList->SetDescriptorHeaps(1, sceneheaps);
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

	//���߂̃N���[�Y
	_cmdList->Close();

	//�R�}���h���X�g�̎��s
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	////�҂�
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() < _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//�L���[���N���A
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);//�ĂуR�}���h���X�g�����߂鏀��
}

ComPtr < IDXGISwapChain4>
Dx12Wrapper::Swapchain() {
	return _swapchain;
}