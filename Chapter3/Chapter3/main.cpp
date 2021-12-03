#include <windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace std;

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
		_T("DX12�e�X�g"),		// �^�C�g���o�[�̕���
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
		if (strDesc.find(L"NVIDIA") != std::string::npos){
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

	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (size_t idx = 0; idx < swcDesc.BufferCount; idx++) {
		// �X���b�v�`�F�[���̒��̃������擾
		result = swapchain_->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
		// �����_�[�^�[�Q�b�g�r���[����
		// ��3�����́A�A�h���X�ł͂Ȃ��n���h���Ȃ̂ŁA�|�C���^�̒P���ȃC���N�������g�͎g�p�ł��Ȃ�
		dev_->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
		// �����_�[�^�[�Q�b�g�r���[�̃T�C�Y���|�C���^�����炷
		// ���̃f�B�X�N���v�^�n���h�����擾�ł���悤�ɂ���
		handle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

/* �t�F���X���� */
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = dev_->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));


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
		result = cmdAllocator_->Reset();
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
		float clearColor[] = { 1.f, 1.f, 0.f,1.f };	// ���F
		cmdList_->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

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
		cmdList_->Reset(cmdAllocator_, nullptr); // �ĂуR�}���h���X�g�����߂鏀��(Close�̉���)

		// �t���b�v
		swapchain_->Present(1, 0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);
}

