#include <windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace std;
using namespace DirectX;

// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// @param format�t�H�[�}�b�g(%d�Ƃ�%f�Ƃ��́j
// @param �ϒ�����
// @remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

// �ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// �E�B���h�E���j�����ꂽ�Ă΂��
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);// OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);// �K��̏������s��
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* dev_ = nullptr;
IDXGIFactory6* dxgiFactory_ = nullptr;
IDXGISwapChain4* swapchain_ = nullptr;
ID3D12CommandAllocator* cmdAllocator_ = nullptr;
ID3D12GraphicsCommandList* cmdList_ = nullptr;
ID3D12CommandQueue* cmdQueue_ = nullptr;

///�A���C�����g�ɑ������T�C�Y��Ԃ�
///@param size ���̃T�C�Y
///@param alignment �A���C�����g�T�C�Y
///@return �A���C�����g�����낦���T�C�Y
size_t
AlignmentedSize(size_t size, size_t alignment) {
	return size + alignment - size % alignment;
}

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

	// �E�B���h�E���쐬���ĕ\������
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;// �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");// �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);// �n���h���̎擾

	RegisterClassEx(&w); // �A�v���P�[�V�����N���X(�E�B���h�E�N���X�̎w���OS�ɓ`����)

	RECT wrc = { 0,0,window_width, window_height }; // �E�B���h�E�T�C�Y�����߂�
	// �֐����g���ăE�B���h�E�T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g����
	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12 �P���|���S���e�X�g"),		// �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	// �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,			// �\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,			// �\��Y���W��OS�ɂ��C��
		wrc.right - wrc.left,	// �E�B���h�E��
		wrc.bottom - wrc.top,	// �E�B���h�E��
		nullptr,				// �e�E�B���h�E�n���h��
		nullptr,				// ���j���[�n���h��
		w.hInstance,			// �Ăяo���A�v���P�[�V�����n���h��
		nullptr);				// �ǉ��p�����[�^

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

#ifdef _DEBUG
	// �f�o�b�O���C���[���I����
	// �f�o�C�X�����O�ɂ���Ă����Ȃ��ƁA�f�o�C�X������ɂ���
	// �f�o�C�X�����X�g���Ă��܂��̂Œ���
	EnableDebugLayer();
#endif // _DEBUG

	/*
	* DirectX������
	*/
	// FEATURE_LEVEL��
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	HRESULT result = S_OK;

	/* D3D12CreateDevice�ɓn���A�_�v�^�[��T�� */
#ifdef _DEBUG
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory_));
#else
	// DXGIFactory�I�u�W�F�N�g�̐���
	CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory_));
#endif // _DEBUG

	// ���p�\�ȃA�_�v�^�[�̗񋓗p
	std::vector<IDXGIAdapter*> adapters;

	// �����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;

	// ���p�\�ȃA�_�v�^�[���
	for (int i = 0;
		dxgiFactory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		adapters.push_back(tmpAdapter);
	}

	// ���p�\�ȃA�_�v�^�[���X�g����~�����A�_�v�^�[��T���ăZ�b�g
	for (auto adpt : adapters) {
		// �A�_�v�^�[�����ʂ��邽�߂̏��(�\����)
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	// �A�_�v�^�[�̐����I�u�W�F�N�g�擾

		std::wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�[�̖��O���m�F
		// �~�����A�_�v�^�[������������Z�b�g
		// ����͖��O��NVIDIA���܂܂�Ă���A�_�v�^�[��T��
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels) {
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&dev_)) == S_OK) {
			featureLevel = lv;
			break;	// �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}
	}

	// �R�}���h���X�g�A���P�[�^�[����
	result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&cmdAllocator_));
	// �R�}���h���X�g����
	result = dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator_, nullptr, IID_PPV_ARGS(&cmdList_));

	/* �R�}���h�L���[ */
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	// �^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//�@�A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.NodeMask = 0;
	// �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	// �R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// �L���[���� 
	result = dev_->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue_));

	/* �X���b�v�`�F�[������ */
	// �X���b�v�`�F�[���̐ݒ���̍\����
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;

	// �o�b�N�o�b�t�@�\�͐L�яk�݉\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	// �t���b�v��͑��₩�ɔj��
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// ���Ɏw��Ȃ�
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// �E�B���h�E�̃t���X�N���[���؂�ւ�
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// �X���b�v�`�F�[������
	result = dxgiFactory_->CreateSwapChainForHwnd(
		cmdQueue_,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapchain_);

	/* �f�B�X�N���v�^�q�[�v���쐬���� */

		// �f�B�X�N���v�^�q�[�v�̐ݒ���̍\����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;	// ���\��2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ���Ɏw��Ȃ�

	ID3D12DescriptorHeap* rtvHeaps = nullptr;

	result = dev_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	/* �f�B�X�N���v�^�ƃX���b�v�`�F�[����̃o�b�t�@�\�Ƃ��֘A�t����(�������̈�̊֘A�t��) */
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	// �X���b�v�`�F�[���̐ݒ�����擾����
	result = swapchain_->GetDesc(&swcDesc);

	// �f�B�X�N���v�^�q�[�v�̐擪�A�h���X�擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//�K���}�␳����(sRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (size_t idx = 0; idx < swcDesc.BufferCount; idx++) {
		// �X���b�v�`�F�[���̒��̃������擾
		result = swapchain_->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		// �����_�[�^�[�Q�b�g�r���[����
		// ��3�����́A�A�h���X�ł͂Ȃ��n���h���Ȃ̂ŁA�|�C���^�̒P���ȃC���N�������g�͎g�p�ł��Ȃ�
		dev_->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
		//dev_->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
		// �����_�[�^�[�Q�b�g�r���[�̃T�C�Y���|�C���^�����炷
		// ���̃f�B�X�N���v�^�n���h�����擾�ł���悤�ɂ���
		handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	/* �t�F���X���� */
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = dev_->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));



	/* �e�N�X�`���}�b�s���O���� */

	struct Vertex {
		XMFLOAT3 pos;//XYZ���W
		XMFLOAT2 uv;//UV���W
	};

	Vertex vertices[] = {
		{{-0.4f,-0.7f,0.0f},{0.0f,1.0f} },//����
		{{-0.4f,0.7f,0.0f} ,{0.0f,0.0f}},//����
		{{0.4f,-0.7f,0.0f} ,{1.0f,1.0f}},//�E��
		{{0.4f,0.7f,0.0f} ,{1.0f,0.0f}},//�E��
	};

	/* ���_�o�b�t�@�\�쐬 */
	D3D12_HEAP_PROPERTIES heapprop = {};

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;
	result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	/* ���_���̃R�s�[ */
	Vertex* vertMap = nullptr;

	result = vertBuff->Map(0, nullptr, (void**)&vertMap);

	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);	// �����}�b�v���������Ă悢

/* ���_�o�b�t�@�\�r���[�̍쐬 */
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// �o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = sizeof(vertices);	// �S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]);	// 1���_������̃o�C�g��

	//�C���f�b�N�X�f�[�^
	unsigned short indices[] = {
		0,1,2,
		2,1,3
	};

	ID3D12Resource* idxBuff = nullptr;

	//�ݒ�́A�o�b�t�@�\�̃T�C�Y�ȊO�A���_�o�b�t�@�[�̐ݒ���g���܂킵�Ă悢
	resdesc.Width = sizeof(indices);

	result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff)
	);

	//�C���f�b�N�X�f�[�^�̃R�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);

	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);
	//�C���f�b�N�X�o�b�t�@�\�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	/* �V�F�[�_�[�̓ǂݍ��݂Ɛ��� */

		// �V�F�[�_�[�I�u�W�F�N�g
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;

	// ���_�V�F�[�_�[�ǂݍ���
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",// �V�F�[�_�[��
		nullptr,	// define�͂Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�̓f�t�H���g
		"BasicVS", "vs_5_0",	// �֐���BasicVS�A�ΏۃV�F�[�_�[��vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�y�эœK���Ȃ�
		0,
		&_vsBlob, &errorBlob);	// �G���[����errorBlob�Ƀ��b�Z�[�W������

	// �V�F�[�_�[�ǂݍ��݂̃G���[�`�F�b�N
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;	// �G���[���b�Z�[�W�󂯎��pstring
			errstr.resize(errorBlob->GetBufferSize()); // �K�v�ȃT�C�Y���m��
			// �f�[�^�R�s�[
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";

			::OutputDebugStringA(errstr.c_str());
		}
		exit(1); // �s�V��������
	}

	// �s�N�Z���V�F�[�_�[�ǂݍ���
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",// �V�F�[�_�[��
		nullptr,	// define�͂Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�̓f�t�H���g
		"BasicPS", "ps_5_0",	// �֐���BasicVS�A�ΏۃV�F�[�_�[��vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�y�эœK���Ȃ�
		0,
		&_psBlob, &errorBlob);	// �G���[����errorBlob�Ƀ��b�Z�[�W������

	// �V�F�[�_�[�ǂݍ��݂̃G���[�`�F�b�N
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;	// �G���[���b�Z�[�W�󂯎��pstring
			errstr.resize(errorBlob->GetBufferSize()); // �K�v�ȃT�C�Y���m��
			// �f�[�^�R�s�[
			std::copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";

			::OutputDebugStringA(errstr.c_str());
		}
		exit(1); // �s�V��������
	}

	/* ���_���C�A�E�g */
	//�O�p�`�̑傫�����T���v���ƈقȂ��Ă������������_���C�A�E�g�ƒ��_�o�b�t�@�\��
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION",//�Z�}���e�B�N�X��
			0,//�����Z�}���e�B�N�X���̂Ƃ��Ɏg���C���f�b�N�X(0�ŗǂ�)
			DXGI_FORMAT_R32G32B32_FLOAT,//�t�H�[�}�b�g(�v�f���ƃh�b�g���Ō^��\��)
			0,//���̓X���b�g�C���f�b�N�X(0�ŗǂ�)
			D3D12_APPEND_ALIGNED_ELEMENT,//�f�[�^�̃I�t�Z�b�g�ʒu
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,//
			0//��x�ɕ`�悷��C���X�^���X�̐�(0�ŗǂ�)
		},
		{	//UV���W
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
	};

	/* �O���t�B�b�N�X�p�C�v���C�� */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr; // ��Őݒ肷��

	// �V�F�[�_�[�̃Z�b�g
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	// �T���v���}�X�N�A���X�^���C�U�[�X�e�[�g�̐ݒ�
	// �f�t�H���g�̃T���v���}�X�N
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;	// ���g��0xffffffff
	// �܂��A���`�G�C���A�X�͎g��Ȃ�����false
	gpipeline.RasterizerState.MultisampleEnable = false;

	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	// ���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;	// �[�x�����̃N���b�s���O�͗L����

	// �u�����h�X�e�[�g�̐ݒ�
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// ���̓��C�A�E�g�̐ݒ�
	gpipeline.InputLayout.pInputElementDescs = inputLayout;	// ���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);	// ���C�A�E�g�z��̗v�f��
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �X�g���b�v���̃J�b�g����
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// �O�p�`�ō\��
	// �����_�[�^�[�Q�b�g�̐ݒ�
	gpipeline.NumRenderTargets = 1; // ����1�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0�`1�ɐ��K�����ꂽRGBA
	// �A���`�G�C���A�V���O�̂��߂̃T���v�����ݒ�
	gpipeline.SampleDesc.Count = 1;	// �T���v�����O��1�s�N�Z���ɂ�1
	gpipeline.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�

/* ���[�g�V�O�l�`���̍쐬 */

	//�f�B�X�N���v�^�����W
	D3D12_DESCRIPTOR_RANGE descTblRange = {};

	descTblRange.NumDescriptors = 1;//�e�N�X�`��1��
	descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//��ʂ̓e�N�X�`��
	descTblRange.BaseShaderRegister = 0;//0�ԃX���b�g����
	descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//���[�g�p�����[�^�[��`
	D3D12_ROOT_PARAMETER rootparam = {};
	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//�s�N�Z���V�F�[�_�[���猩����
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//�f�B�X�N���v�^�����W�̃A�h���X
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
	//�f�B�X�N���v�^�����W��
	rootparam.DescriptorTable.NumDescriptorRanges = 1;

	//�T���v���[�̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};

	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�������̌J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�����̌J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s���̌J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;//�{�[�_�[�͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//���`���
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.f;//�~�b�v�}�b�v�ŏ��l
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_�[���猩����
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//���T���v�����O���Ȃ�

	//���[�g�V�O�l�`���쐬
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	// ���_��񂪂���
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = &rootparam;//���[�g�p�����[�^�[�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 1;//���[�g�p�����[�^�[��
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	// �o�C�i���R�[�h�쐬
	ID3DBlob* rootSigBlob = nullptr;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ���[�g�V�O�l�`���ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0,	// ���[�g�V�O�l�`���o�[�W����
		&rootSigBlob,	// �V�F�[�_�[����������Ɠ���
		&errorBlob);	// �G���[����������

	// ���[�g�V�O�l�`���I�u�W�F�N�g�̍쐬
	result = dev_->CreateRootSignature(
		0,	// nodemask 0�ł悢
		rootSigBlob->GetBufferPointer(),	// �V�F�[�_�[�̂Ƃ��Ɠ��l
		rootSigBlob->GetBufferSize(),		// �V�F�[�_�[�̂Ƃ��Ɠ��l
		IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	ID3D12PipelineState* _pipelineState = nullptr;
	result = dev_->CreateGraphicsPipelineState(
		&gpipeline, IID_PPV_ARGS(&_pipelineState));

	/* �r���[�|�[�g�ƃV�U�[��` */
	// �r���[�|�[�g
	D3D12_VIEWPORT viewport = {};

	viewport.Width = window_width;		// �o�͐�̕�(�s�N�Z����)
	viewport.Height = window_height;	// �o�͐�̍���(�s�N�Z����)
	viewport.TopLeftX = 0;	// �o�͐�̍�����WX
	viewport.TopLeftY = 0;	// �o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;	// �[�x�ő�l
	viewport.MinDepth = 0.f;	// �[�x�ŏ��l

	// �V�U�[��`
	D3D12_RECT scissorrect = {};

	scissorrect.top = 0;	// �؂蔲������W
	scissorrect.left = 0;	// �؂蔲�������W
	scissorrect.right = scissorrect.left + window_width;//�؂蔲���E���W
	scissorrect.bottom = scissorrect.top + window_height;//�؂蔲�������W

	//WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = CoInitializeEx(0, COINIT_MULTITHREADED);
	//result = LoadFromWICFile(L"img/ironman.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	//result = LoadFromWICFile(L"img/bird.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	result = LoadFromWICFile(L"img/textest200x200.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	//result = LoadFromWICFile(L"img/textest.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	auto img = scratchImg.GetImage(0, 0, 0);//���f�[�^���o

	//�܂��͒��ԃo�b�t�@�Ƃ��Ă�Upload�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES uploadHeapProp = {};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//Upload�p
	//�A�b�v���[�h�p�Ɏg�p���邱�ƑO��Ȃ̂�UNKOWN�ł悢
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂��߂�0
	uploadHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂��߂�0

	D3D12_RESOURCE_DESC uploadResDesc = {};
	uploadResDesc.Format = DXGI_FORMAT_UNKNOWN;//�P�Ȃ�f�[�^�̉�Ȃ̂Ŏw�肵�Ȃ�
	uploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;//�P�Ȃ�o�b�t�@�Ƃ���
	uploadResDesc.Width = AlignmentedSize(img->rowPitch,D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;//�f�[�^�T�C�Y
	uploadResDesc.Height = 1;
	uploadResDesc.DepthOrArraySize = 1;
	uploadResDesc.MipLevels = 1;
	uploadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//�A�������f�[�^
	uploadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//���Ƀt���O����
	uploadResDesc.SampleDesc.Count = 1;//�ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	uploadResDesc.SampleDesc.Quality = 0;

	//���ԃo�b�t�@�\�쐬
	ID3D12Resource* uploadbuff = nullptr;
	result = dev_->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&uploadResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,//CPU���珑�����݉\
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);

	//���Ƀe�N�X�`���̂��߂̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;//�e�N�X�`���p
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0; //�P��A�_�v�^�̂���0

	//���\�[�X�ݒ�(�ϐ��͎g���܂킵)
	uploadResDesc.Format = metadata.format;
	uploadResDesc.Width = metadata.width;
	uploadResDesc.Height = metadata.height;
	uploadResDesc.DepthOrArraySize = metadata.arraySize;
	uploadResDesc.MipLevels = metadata.mipLevels;
	uploadResDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	uploadResDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//�e�N�X�`���o�b�t�@�쐬
	ID3D12Resource* texbuff = nullptr;
	result = dev_->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&uploadResDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,//�R�s�[��
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);
	//8�r�b�g=1�o�C�g
	uint8_t* mapforImg = nullptr;//image->pixels�Ɠ����^�ɂ���
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//�}�b�v
	auto srcAddress = img->pixels;//200 * 200 * 1 * 4
	auto p = img->rowPitch;	//800 = uint8(1�o�C�g) * 4(RGBA) * 200��
	auto height = img->height;
	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	for (int y = 0; y < img->height; ++y) {
		std::copy_n(srcAddress, 
			rowPitch, 
			mapforImg);//�R�s�[
		//1�s���Ƃ̒�������킹�Ă��
		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}
	uploadbuff->Unmap(0, nullptr);//�A���}�b�v

	D3D12_TEXTURE_COPY_LOCATION src = {};

	//�R�s�[��(�A�b�v���[�h��)�ݒ�
	src.pResource = uploadbuff;//���ԃo�b�t�@�\
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;//�t�b�g�v�����g�w��
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = metadata.width;
	src.PlacedFootprint.Footprint.Height = metadata.height;
	src.PlacedFootprint.Footprint.Depth = metadata.depth;
	src.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch,D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	src.PlacedFootprint.Footprint.Format = img->format;

	D3D12_TEXTURE_COPY_LOCATION dst = {};

	//�R�s�[��ݒ�
	dst.pResource = texbuff;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	{
		cmdList_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = texbuff;
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;//�d�v
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;//�d�v

		cmdList_->ResourceBarrier(1, &BarrierDesc);
		cmdList_->Close();

		//�R�}���h���X�g���s
		ID3D12CommandList* cmdlists[] = { cmdList_ };
		cmdQueue_->ExecuteCommandLists(1, cmdlists);

		cmdQueue_->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		cmdAllocator_->Reset();//�L���[���N���A
		cmdList_->Reset(cmdAllocator_, nullptr);
	}


	//�f�B�X�N���v�^�q�[�v�쐬
	ID3D12DescriptorHeap* texDescHeap = {};
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	//�V�F�[�_�[���猩����悤��
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//�}�X�N��0
	descHeapDesc.NodeMask = 0;
	//�r���[�͍��̂Ƃ���P����
	descHeapDesc.NumDescriptors = 1;
	//����
	result = dev_->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));

	//�V�F�[�_�[���\�[�X�r���[�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//�f�[�^��RGBA���ǂ̂悤�Ƀ}�b�s���O���邩
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//�e�N�X�`��2D
	srvDesc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1

	dev_->CreateShaderResourceView(
		texbuff,//�r���[�Ɗ֘A�t����o�b�t�@�\
		&srvDesc,//��قǐݒ肵���e�N�X�`���ݒ���
		texDescHeap->GetCPUDescriptorHandleForHeapStart()//�q�[�v�̂ǂ��Ɋ��蓖�Ă邩
	);

	/* ���C�����[�v */
	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// �A�v���P�[�V�������I��鎞��message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}

		// DirectX����
		// �R�}���h�A���P�[�^�[�̖��߃��X�g�����Z�b�g
		//result = cmdAllocator_->Reset();
		// ���݂̃o�b�t�@�\�C���f�b�N�X�擾
		// ���̃t���[���ŕ`�悳���o�b�N�o�b�t�@�̃C���f�b�N�X
		// ���Ȃ킿�A�����
		auto bbIdx = swapchain_->GetCurrentBackBufferIndex();

		// �o���A
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// �J��
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;	// ���Ɏw��Ȃ�
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];	// �o�b�N�o�b�t�@�\���\�[�X
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;	// ���O��PRESENT���
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// �����烌���_�[�^�[�Q�b�g���
		cmdList_->ResourceBarrier(1, &BarrierDesc);

		// �����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		cmdList_->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// ��ʃN���A
		float clearColor[] = { 0.f, 0.f, 0.f, 1.f };	// ���F
		cmdList_->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		cmdList_->SetPipelineState(_pipelineState);
		cmdList_->SetGraphicsRootSignature(rootsignature);

		cmdList_->RSSetViewports(1, &viewport);
		cmdList_->RSSetScissorRects(1, &scissorrect);

		//cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList_->IASetVertexBuffers(0, 1, &vbView);
		cmdList_->IASetIndexBuffer(&ibView);

		//�e�N�X�`���`��ɕK�v
		cmdList_->SetGraphicsRootSignature(rootsignature);
		cmdList_->SetDescriptorHeaps(1, &texDescHeap);
		cmdList_->SetGraphicsRootDescriptorTable(0, texDescHeap->GetGPUDescriptorHandleForHeapStart());

		//cmdList_->DrawInstanced(4, 1, 0, 0);
		cmdList_->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// �O��̏�ԓ���ւ�
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList_->ResourceBarrier(1, &BarrierDesc);

		// ���߂̃N���[�Y
		// ���߂̎�t�����߂�
		cmdList_->Close();

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* cmdlists[] = { cmdList_ };
		cmdQueue_->ExecuteCommandLists(1, cmdlists);
		// �҂�
		cmdQueue_->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			// �C�x���g����������܂ő҂�������(INFINITE)
			WaitForSingleObject(event, INFINITE);
			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		cmdAllocator_->Reset();	// �L���[���N���A
		cmdList_->Reset(cmdAllocator_, _pipelineState); // �ĂуR�}���h���X�g�����߂鏀��(Close�̉���)

		// �t���b�v
		swapchain_->Present(1, 0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);
}

