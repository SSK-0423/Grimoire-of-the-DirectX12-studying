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

	//DXGI�܂��
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;

	//DirectX12�܂��
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;

	//�����_�[�^�[�Q�b�g�֘A
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _backBuffers;	//�o�b�N�o�b�t�@
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH;

	//�r���[�|�[�g���V�U�[��`
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorrect = {};

	//�f�v�X�X�e���V���r���[�֘A
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer = nullptr;	//�[�x�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

	//�V�[�����\������o�b�t�@�܂��
	Microsoft::WRL::ComPtr<ID3D12Resource> _sceneConstBuff = nullptr;

	struct SceneData
	{
		DirectX::XMMATRIX view;	//�r���[�s��
		DirectX::XMMATRIX proj;	//�v���W�F�N�V�����s��
		DirectX::XMFLOAT3 eye;//���_���W
	};
	SceneData* _mappedSceneData;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;

	//�t�F���X
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	//�o���A
	D3D12_RESOURCE_BARRIER BarrierDesc = {};

	//�f�o�C�X��DXGI�̐���
	HRESULT CreateDeviceAndDXGIFactory();
	//�X���b�v�`�F�[������
	HRESULT CreateSwapChain(HWND& hwnd, const SIZE& size);
	//�R�}���h�A���P�[�^�A���X�g�A�L���[����
	HRESULT CreateCommandX();
	//�����_�[�^�[�Q�b�g�r���[����
	HRESULT CreateFinalRenderTarget(const SIZE& size);
	////�f�v�X�X�e���V���r���[����
	HRESULT CreateDepthStencilView(const SIZE& size);
	//�t�F���X�쐬
	HRESULT CreateID3D12Fence();

	//�r���[�v���W�F�N�V�����p�r���[�̐���
	HRESULT	CreateSceneView();

	//�e�N�X�`���ǂݍ��݂̃����_��
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> _loadLambdaTable;
	//�e�N�X�`���e�[�u��
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _textureTable;
	//�e�N�X�`�����[�_�e�[�u���쐬
	void CreateTextureLoaderTable();
	//�t�@�C������e�N�X�`������
	ID3D12Resource* CreateTextureFromFile(const char* texpath);

public:
	Dx12Wrapper(HWND& hwnd);
	~Dx12Wrapper();

	void Update();
	void EndDraw();
	void BeginDraw();

	///�e�N�X�`���p�X����K�v�ȃe�N�X�`���o�b�t�@�ւ̃|�C���^��Ԃ�
	///@param texpath �e�N�X�`���t�@�C���p�X
	Microsoft::WRL::ComPtr<ID3D12Resource> GetTextureByPath(const char* texPath);

	Microsoft::WRL::ComPtr<ID3D12Device> Device();	//�f�o�C�X
	Microsoft::WRL::ComPtr<IDXGISwapChain4> Swapchain();	//�R�}���h���X�g
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList();	//�X���b�v�`�F�[��

	void SetScene();
};