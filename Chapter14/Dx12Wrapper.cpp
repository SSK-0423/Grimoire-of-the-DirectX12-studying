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
HRESULT Dx12Wrapper::CreateEffectBufferAndView()
{
	//�@���}�b�v�����[�h����
	_effectTexBuffer = CreateTextureFromFile("normal/crack_n.png");
	if (_effectTexBuffer == nullptr) {
		assert(0);
		return E_FAIL;
	}

	//�|�X�g�G�t�F�N�g�p�̃f�B�X�N���v�^�q�[�v����
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

	//�|�X�g�G�t�F�N�g�p�e�N�X�`���r���[�����
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = _effectTexBuffer->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	_dev->CreateShaderResourceView(_effectTexBuffer.Get(), &srvDesc, _effectSRVHeap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

//�u���[���p���\�[�X�쐬
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
		resDesc.Width >>= 1;	//�E�V�t�g(1/2)
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
	resDesc.Width >>= 1;	//�k���o�b�t�@�[�Ȃ̂ő傫���𔼕��ɂ���
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
	//�f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif

	auto& app = Application::Instance();
	_winSize = app.GetWindowSize();

	//�e�N�X�`�����[�_�[�֘A������
	CreateTextureLoaderTable();

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

	//�u���[���̃o�b�t�@�[����
	if (FAILED(CreateBloomBufferAndView()))
	{
		assert(0);
		return;
	}
	//��ʊE�[�x�̃o�b�t�@�[����
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

	//�[�x�o�b�t�@�쐬
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return;
	}

	//�[�x�l���e�N�X�`���Ƃ��ė��p����
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
	if (FAILED(result)) {
		return result;
	}
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
	resdesc.Format = DXGI_FORMAT_R32_TYPELESS;	//�r�b�g���̂ݎw��
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resdesc.MipLevels = 1;
	resdesc.Alignment = 0;

	//�f�v�X�p�q�[�v�v���p�e�B
	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.f, 0);
	//�[�x�o�b�t�@�\����
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

	//�V���h�E�}�b�v�p�̐[�x�o�b�t�@�\����
	resdesc.Width = shadow_difinition;
	resdesc.Height = shadow_difinition;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //�f�v�X�������݂Ɏg�p
		&depthClearValue,
		IID_PPV_ARGS(_lightDepthBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		//�G���[����
		return result;
	}

	//�[�x�̂��߂̃f�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//�[�x�Ɏg����Ƃ��������킩��΂���
	dsvHeapDesc.NumDescriptors = 2;//�ʏ�{�V���h�E�}�b�v�p�[�x
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//�f�v�X�X�e���V���r���[�Ƃ��Ďg��
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//�[�x�p�̃f�B�X�N���v�^�q�[�v����
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return result;
	}

	//�ʏ�[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//�f�v�X�l��32bit�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//�t���O�͓��ɂȂ�

	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, handle);
	//���C�g�[�x����
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_dev->CreateDepthStencilView(_lightDepthBuffer.Get(), &dsvDesc, handle);


	return result;
}

//SRV�p�̃f�v�X
HRESULT Dx12Wrapper::CreateDepthSRV()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 2;
	heapDesc.NodeMask = 0;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//�[�xSRV�̃f�B�X�N���v�^�q�[�v����
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
	//�ʏ�[�x���e�N�X�`���p
	auto handle = _depthSRVHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateShaderResourceView(_depthBuffer.Get(), &srvDesc, handle);

	//���C�g�[�x���e�N�X�`���p
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
	XMFLOAT4 planeNormalVec(0, 1, 0, 0);	// ���ʂ̕�����
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��45��
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//�A�X��
		1.f,//�߂���
		100.0f//������
	);

	auto eyePos = XMLoadFloat3(&eye);
	auto targetPos = XMLoadFloat3(&target);
	auto upVec = XMLoadFloat3(&up);

	auto light = XMFLOAT4(-1, 1, -1, 0);
	XMVECTOR lightVec = XMLoadFloat4(&light);
	auto lightPos = targetPos + XMVector3Normalize(lightVec) * XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_mappedSceneData->lightCamera = XMMatrixLookAtLH(lightPos, targetPos, upVec) * XMMatrixOrthographicLH(40, 40, 1.f, 100.f);
	_mappedSceneData->shadow = XMMatrixShadow(XMLoadFloat4(&planeNormalVec), -XMLoadFloat3(&_perallelLightvVec));	//�e�s��쐬
	_mappedSceneData->eye = eye;
}

std::vector<float> Dx12Wrapper::GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);	//�E�F�C�g�z��ԋp�p
	float x = 0.f;
	float total = 0.f;
	for (auto& wgt : weights)
	{
		wgt = expf(-(x * x) / (2 * s * s));
		total += wgt;
		x += 1.f;
	}

	total = total * 2.f - 1;
	//������1�ɂȂ�悤�ɂ���
	for (auto& wgt : weights)
	{
		wgt /= total;
	}

	return weights;
}

//
// �q�[�v�v���p�e�B�ݒ�
// ���\�[�X�ݒ�
// ����
// �}�b�v
// �r���[�ݒ�
// �r���[����
//
HRESULT Dx12Wrapper::CreateGaussianConstantBufferAndView()
{
	_gaussianWeights = GetGaussianWeights(8, 5);
	//�o�b�t�@�\����
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

	//�}�b�v����
	float* mappedWeight = nullptr;
	result = _gaussianBuff->Map(0, nullptr, (void**)&mappedWeight);
	if (FAILED(result))
	{
		assert(0);
		return result;
	}
	std::copy(_gaussianWeights.begin(), _gaussianWeights.end(), mappedWeight);
	_gaussianBuff->Unmap(0, nullptr);

	//�r���[����
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv = {};
	cbv.BufferLocation = _gaussianBuff->GetGPUVirtualAddress();
	cbv.SizeInBytes = _gaussianBuff->GetDesc().Width;

	//CreatePeraResourceAndView���\�b�h��ύX�����ۂ͒���
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

	XMFLOAT3 eye(0, 15, -25);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	XMFLOAT4 planeNormalVec(0, 1, 0, 0);	// ���ʂ̕�����
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��45��
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//�A�X��
		1.f,//�߂���
		100.0f//������
	);

	auto eyePos = XMLoadFloat3(&eye);
	auto targetPos = XMLoadFloat3(&target);
	auto upVec = XMLoadFloat3(&up);

	auto light = XMFLOAT4(-1, 1, -1, 0);
	XMVECTOR lightVec = XMLoadFloat4(&light);
	auto lightPos = targetPos + XMVector3Normalize(lightVec) * XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];
	_mappedSceneData->lightCamera = XMMatrixLookAtLH(lightPos,targetPos,upVec) * XMMatrixOrthographicLH(40, 40, 1.f, 100.f);
	_mappedSceneData->shadow = XMMatrixShadow(XMLoadFloat4(&planeNormalVec), -XMLoadFloat3(&_perallelLightvVec));	//�e�s��쐬
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
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());	//���_�V�F�[�_�[�ݒ�
	//plsDesc.PS.BytecodeLength = 0;	//�s�N�Z���V�F�[�_�[�K�v�Ȃ�
	//plsDesc.PS.pShaderBytecode = nullptr; //�s�N�Z���V�F�[�_�[�K�v�Ȃ�
	plsDesc.NumRenderTargets = 1;
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;;
	plsDesc.pRootSignature = _peraRootSignature.Get();
	plsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	result = _dev->CreateGraphicsPipelineState(&plsDesc, IID_PPV_ARGS(_shadowPipline.ReleaseAndGetAddressOf()));

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
	//float clsClr[] = { 0.f,0.f,0.f,1.0f };//���F
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clsClr);

	//1�p�X�ڂ̓}���`�����_�[�^�[�Q�b�g
	for (auto& pera : _peraResource)
	{
		auto result = _dev->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			//D3D12_RESOURCE_STATE_RENDER_TARGET�ł͂Ȃ�
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearValue,
			IID_PPV_ARGS(pera.ReleaseAndGetAddressOf())
		);

		if (FAILED(result)) {
			assert(0);
			return result;
		}
	}

	//2�p�X��
	auto result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		//D3D12_RESOURCE_STATE_RENDER_TARGET�ł͂Ȃ�
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(_peraResource2.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//RTV�p�q�[�v�����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = _rtvHeaps->GetDesc();
	heapDesc.NumDescriptors = 6;	//1�p�X��(4��) 2�p�X��(1��)
	result = _dev->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(_peraRTVHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�����_�[�^�[�Q�b�g�r���[�����
	//1,2����
	auto handle = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& pera : _peraResource)
	{
		_dev->CreateRenderTargetView(
			pera.Get(), &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	//3����(�u���[��)
	_dev->CreateRenderTargetView(
		_bloomBuffer[0].Get(), &rtvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	//4����(�k���o�b�t�@�[)
	_dev->CreateRenderTargetView(
		_bloomBuffer[1].Get(), &rtvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//5����(��ʊE�[�x)
	_dev->CreateRenderTargetView(
		_dofBuffer.Get(), &rtvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//6����
	_dev->CreateRenderTargetView(
		_peraResource2.Get(), &rtvDesc, handle);

	//SRV�p�q�[�v�����
	//1�`5�y���|���S��1�p 6�y���|���S��2�p + �K�E�X�ڂ���
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

	//�V�F�[�_�[���\�[�X�r���[�����
	//1�p�X��(1,2��)
	handle = _peraSRVHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& pera : _peraResource)
	{
		_dev->CreateShaderResourceView(
			pera.Get(), &srvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	//3����(�u���[��)
	_dev->CreateShaderResourceView(
		_bloomBuffer[0].Get(), &srvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//4����(�k���o�b�t�@�[)
	_dev->CreateShaderResourceView(
		_bloomBuffer[1].Get(), &srvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//5����(��ʊE�[�x�p�k���o�b�t�@�[)
	_dev->CreateShaderResourceView(
		_dofBuffer.Get(), &srvDesc, handle);
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//2�p�X��(6����)
	_dev->CreateShaderResourceView(
		_peraResource2.Get(), &srvDesc, handle);

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

	D3D12_DESCRIPTOR_RANGE range[4] = {};
	//�O�p�X�̃����_�����O����
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range[0].BaseShaderRegister = 0;	// t0�`t4
	range[0].NumDescriptors = 5; //�}���`�����_�[�^�[�Q�b�g(3��)�Ȃ̂�3

	//�G�t�F�N�g�p�o�b�t�@�[
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	// b
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;

	//�G�t�F�N�g�p�̃e�N�X�`��
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range[2].BaseShaderRegister = 5;	// 1
	range[2].NumDescriptors = 1;
	
	//�[�x�\���p�e�N�X�`��
	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	// t
	range[3].BaseShaderRegister = 6;	//2
	range[3].NumDescriptors = 2;	// �g���̂�1��

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

	//���[�g�V�O�l�`������
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
	//�p�C�v���C���X�e�[�g����
	//1����
	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//2����
	//�s�N�Z���V�F�[�_�[�R���p�C��
	result = D3DCompileFromFile(
		L"VerticalBokehPS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VerticalBokehPS", "ps_5_0",
		0, 0, ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	//�p�C�v���C���X�e�[�g����
	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(_peraPipeline2.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//�ڂ����p�p�C�v���C������
	result = D3DCompileFromFile(
		L"peraPixel.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BlurPS", "ps_5_0",
		0, 0, ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		assert(0);
		return result;
	}
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	//�u���[���k���Ɣ�ʊE�[�x�k��
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

//1�p�X�ڂ̕`�揀��(�ʏ�`��)
void Dx12Wrapper::PreDrawRenderTarget1()
{
	//�o���A�ݒ�
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

	//�����_�[�^�[�Q�b�g�Z�b�g
	uint32_t offset = 0;
	for (auto& handle : handles)
	{
		handle.InitOffsetted(baseHandle, offset);
		offset += incSize;
	}

	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//1�p�X�ڂ̃����_�[�^�[�Q�b�g�Z�b�g
	//���ʂ���̐[�x�Z�b�g
	_cmdList->OMSetRenderTargets(_countof(handles), handles, false, &dsvH);	//�[�x�l���������܂��悤�ɂȂ�

	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	for (size_t i = 0; i < _countof(handles); i++)
	{
		//���P�x�̏ꍇ
		if (i == 2) {
			float clearColor[] = { 0.f,0.f,0.f,1.f };
			_cmdList->ClearRenderTargetView(handles[i], clearColor, 0, nullptr);
			continue;
		 }
		_cmdList->ClearRenderTargetView(handles[i], clearColor, 0, nullptr);
	}
	//�[�x�N���A
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());

	//�[�xSRV�Z�b�g
	//���C�g�n�_�̐[�x��
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	auto handle = _depthSRVHeap->GetGPUDescriptorHandleForHeapStart();
	//���C�g����̐[�x���g�p���邽��2�ڂ̃r���[���g��
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(3, handle);
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

//�y���|���S��2���ڕ`�揀��
void Dx12Wrapper::PreDrawRenderTarget2()
{
	//�o���A�ݒ�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_peraResource2.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	auto rtvHeapPointer = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	//2�p�X�ڂ������_�[�^�[�Q�b�g�ɂ���
	rtvHeapPointer.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//2�p�X�ڂ̃����_�[�^�[�Q�b�g�Z�b�g
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);

	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	//�[�x�N���A
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());

	//1���ڃy���|���S���p�̕`��p�C�v���C���ɍ��킹��
	_cmdList->SetPipelineState(_peraPipeline.Get());
	//����
	_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());
	//�v���~�e�B�u�g�|���W�[�ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//�y���|���S��2���ڕ`��(X�����K�E�V�A���t�B���^�K�p)
void Dx12Wrapper::DrawRenderTarget2()
{
	//�܂��q�[�v���Z�b�g
	_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	auto handle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	//�p�����[�^�[0�Ԃƃq�[�v���֘A�t����
	//1�p�X�ڂ̕`�挋��(�ʏ�`��)���e�N�X�`���Ƃ��đ��M����
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	//�R���X�^���g�o�b�t�@�\���Z�b�g
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;
	_cmdList->SetGraphicsRootDescriptorTable(1, handle);
	
	//�[�x���e�N�X�`���Ƃ��ė��p���邽�߂̃f�B�X�N���v�^�q�[�v�Z�b�g
	_cmdList->SetDescriptorHeaps(1, _depthSRVHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(3, _depthSRVHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

//�y���|���S��2���ڕ`��I��(X�����K�E�V�A���t�B���^�K�p)
void Dx12Wrapper::EndDrawRenderTarget2()
{
	//�o���A�ݒ�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_peraResource2.Get(),
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
	//�����_�[�^�[�Q�b�g�Z�b�g
	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);

	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);
	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());
	//�ŏI�����_�����O�p�̃p�C�v���C���Z�b�g
	_cmdList->SetPipelineState(_peraPipeline.Get());
	_cmdList->SetGraphicsRootSignature(_peraRootSignature.Get());
}

//�ŏI�����_�����O���ʂ̕`��
//�o�b�N�o�b�t�@�\�ւ̕`��
void Dx12Wrapper::DrawFinalRenderTarget()
{
	//�܂��q�[�v���Z�b�g
	//1���ڂ̃y���|���S���̕`�挋�ʂ��e�N�X�`���Ƃ��Ď擾
	_cmdList->SetDescriptorHeaps(1, _peraSRVHeap.GetAddressOf());
	auto handle = _peraSRVHeap->GetGPUDescriptorHandleForHeapStart();

	//����őO�p�X�̃����_�����O���ʂ��e�N�X�`���Ƃ��ė��p�ł���(�V�F�[�_�[�ɓn�����)
	_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	//�R���X�^���g�o�b�t�@�\�̃f�B�X�N���v�^�R�Â�
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	//�G�t�F�N�g�p�e�N�X�`��
	_cmdList->SetDescriptorHeaps(1, _effectSRVHeap.GetAddressOf());
	_cmdList->SetGraphicsRootDescriptorTable(2, _effectSRVHeap->GetGPUDescriptorHandleForHeapStart());
	
	//�[�x���e�N�X�`���Ƃ��ė��p���邽�߂̃f�B�X�N���v�^�q�[�v�Z�b�g
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

	//���_�o�b�t�@�[�Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peraVBV);

	//�k���o�b�t�@�[�̓����_�[�^�[�Q�b�g��
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
	//4�ڂɈړ�(�u���[���k���o�b�t�@�[)
	rtvHandles[0].InitOffsetted(rtvBaseHandle, rtvIncSize * 3);
	//5�ڂɈړ�(��ʊE�[�x�k���o�b�t�@�[)
	rtvHandles[1].InitOffsetted(rtvBaseHandle, rtvIncSize * 4);
	//�����_�[�^�[�Q�b�g�Z�b�g
	_cmdList->OMSetRenderTargets(2, rtvHandles, false, nullptr);

	//�e�N�X�`����1���ڂ�3�ڂ̃����_�[�^�[�Q�b�g(���P�x)
	//srvHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;

	//1�p�X�ڂ̍��P�x���e�N�X�`���Ƃ��Ďg�p
	//�V�F�[�_�[����texHighLum���g�p����
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

	//�����Ńr���[�|�[�g�A�V�U�[��`��ύX���Ă���̂ŁA
	//�ŏI�����_�����O�O�ɍĐݒ�K�v���聩�n�}����
	for (int i = 0; i < 8; ++i)
	{
		_cmdList->RSSetViewports(1, &vp);
		_cmdList->RSSetScissorRects(1, &sr);
		_cmdList->DrawInstanced(4, 1, 0, 0);

		//�������牺�ɂ��炵�Ď�����������
		sr.top += vp.Height;
		vp.TopLeftX = 0;
		vp.TopLeftY = sr.top;

		//����������������
		vp.Width /= 2;
		vp.Height /= 2;
		sr.bottom = sr.top + vp.Height;
	}

	//���P�x�����o�b�t�@�[�̓V�F�[�_�[���\�[�X��
	//�k���o�b�t�@�[���V�F�[�_�[���\�[�X��
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_bloomBuffer[1].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	_cmdList->ResourceBarrier(1, &barrier);
}

//�V���h�E�`�揀��
void Dx12Wrapper::PreDrawShadow()
{
	auto handle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	//���C�g�p�̃r���[�ɐ؂�ւ�
	handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//�����_�[�^�[�Q�b�g�Ȃ�
	_cmdList->OMSetRenderTargets(0, nullptr, false, &handle);	//���C�g����̐[�x�Z�b�g
	//�[�x�o�b�t�@�N���A
	_cmdList->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	D3D12_VIEWPORT vp = CD3DX12_VIEWPORT(0.0f, 0.0f, shadow_difinition, shadow_difinition);
	_cmdList->RSSetViewports(1, &vp);//�r���[�|�[�g

	CD3DX12_RECT rc(0, 0, shadow_difinition, shadow_difinition);
	_cmdList->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`
}

//�V���h�E�`��C��
void Dx12Wrapper::EndDrawShadow()
{
}

void
Dx12Wrapper::BeginDraw() {
	//DirectX����
	//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	//�����_�[�^�[�Q�b�g���w��
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto rtvHeapPointer = _peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();

	//1�p�X��
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_cmdList->ResourceBarrier(1, &barrier);

	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	_cmdList->ClearRenderTargetView(rtvHeapPointer, clearColor, 0, nullptr);

	_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvH);	//���ʂ���̐[�x�Z�b�g
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());
}

void
Dx12Wrapper::SetScene() {
	//���݂̃V�[��(�r���[�v���W�F�N�V����)���Z�b�g
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