#pragma once
#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>

#include<d3dcompiler.h>
#include<DirectXTex.h>
#include<d3dx12.h>
#include <wrl.h>
#include "Application.h"

#ifdef _DEBUG
#include<iostream>
#endif
#include <map>

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"

//�֐��}�N��
#define RETURN_FAILED(result) if(FAILED(result)){ assert(0); return result; }

class Application
{
private:
	//�����ɕK�v�ȕϐ�(�o�b�t�@�\��q�[�v�Ȃ�)������
	const unsigned int window_width = 1280;
	const unsigned int window_height = 720;

	HRESULT result;

	//�E�B���h�E
	HWND hwnd;
	WNDCLASSEX windowClass;

	//DXGI�܂��
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;

	//DirectX12�܂��
	Microsoft::WRL::ComPtr<ID3D12Device> _dev = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _cmdAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _cmdList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _cmdQueue = nullptr;

	//�o�b�t�@�\
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer = nullptr;	//�[�x�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff = nullptr;		//���_�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12Resource> idxBuff = nullptr;		//�C���f�b�N�X�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuff = nullptr;		//�萔�o�b�t�@

	//�����_�[�^�[�Q�b�g�r���[�֘A
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeaps = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _backBuffers;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvH;

	//�r���[�|�[�g���V�U�[��`
	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorrect = {};

	//�f�v�X�X�e���V���r���[�֘A
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

	//�r���[
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	//���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {};	//�C���f�b�N�X�o�b�t�@�r���[

	//�}�e���A���܂��
	unsigned int materialNum;	// �}�e���A����
	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> materialDescHeap = nullptr;

	//�f�t�H���g�e�N�X�`��
	Microsoft::WRL::ComPtr<ID3D12Resource> whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> gradTex = nullptr;

	//�V�F�[�_�[�I�u�W�F�N�g
	Microsoft::WRL::ComPtr<ID3DBlob> _vsBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> _psBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	//�e�N�X�`���ǂݍ��݂̃����_��
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map<std::string, LoadLambda_t> LoadLambdaTable;

	//�t�@�C�����p�X�ƃ��\�[�X�̃}�b�v�e�[�u��
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> _resourceTable;

	//���[�g�V�O�l�`�����O���t�B�b�N�X�p�C�v���C��
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootsignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> basicDescHeap = nullptr;

	DirectX::XMMATRIX worldMat;
	DirectX::XMMATRIX viewMat;
	DirectX::XMMATRIX projMat;

	//�V�F�[�_�[���ɓ�������}�e���A���f�[�^
	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;
		float alpha;
		DirectX::XMFLOAT3 specular;
		float specularity;
		DirectX::XMFLOAT3 ambient;
	};

	// ����ȊO�̃}�e���A���f�[�^
	struct AdditionalMaterial
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	// �S�̂��܂Ƃ߂�f�[�^
	struct Material
	{
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	std::vector<Material> materials;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> toonResources;

	//�V�F�[�_���ɓn�����߂̊�{�I�ȍs��f�[�^
	struct SceneMatrix {
		DirectX::XMMATRIX world;	//���[���h�s��
		DirectX::XMMATRIX view;	//�r���[�s��
		DirectX::XMMATRIX proj;	//�v���W�F�N�V�����s��
		DirectX::XMFLOAT3 eye;	//���_���W
	};
	SceneMatrix* mapMatrix;//�}�b�v��������|�C���^

	//�t�F���X
	Microsoft::WRL::ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

#pragma pack(push, 1)
	struct PMD_VERTEX
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
		uint16_t bone_no[2];
		uint8_t  weight;
		uint8_t  EdgeFlag;
		uint16_t dummy;
	};
#pragma pack(pop)

#pragma pack(1)	//�A���C�����g�}��
	//PMD�}�e���A���\����
	struct PMDMaterial
	{
		DirectX::XMFLOAT3 diffuse;	//�f�B�t���[�Y
		float alpha;		//�f�B�t���[�Y��
		float specularity;	//�X�y�L�����̋����i��Z�l�j
		DirectX::XMFLOAT3 specular;	//�X�y�L�����F
		DirectX::XMFLOAT3 ambient;	//�A���r�G���g�F
		unsigned char toonIdx;	//�g�D�[���ԍ�
		unsigned char edgeFlg;	//�}�e���A�����Ƃ̗֊s���t���O
		//�����܂�46
		//���ӁF������2�o�C�g�̃p�f�B���O������

		unsigned int indicesNum;	//���̃}�e���A�������蓖�Ă���
									//�C���f�b�N�X��
		char texFilePath[20];		//�e�N�X�`���t�@�C���p�X+��
	};//70�o�C�g�̂͂������A�p�f�B���O���������邽��72�o�C�g�ɂȂ�
#pragma pack()

	//PMD�w�b�_�\����
	struct PMDHeader {
		float version; //��F00 00 80 3F == 1.00
		char model_name[20];//���f����
		char comment[256];//���f���R�����g
	};



	//DirectX12�̏�����
	//�E�B���h�E����
	void CreateGameWindow();
	//�f�o�C�X��DXGI�̐���
	HRESULT CreateDeviceAndDXGIFactory();
	//�X���b�v�`�F�[������
	HRESULT CreateSwapChain();
	//�R�}���h�A���P�[�^�A���X�g�A�L���[����
	HRESULT CreateCommandX();
	//�����_�[�^�[�Q�b�g�r���[����
	HRESULT CreateFinalRenderTarget();

	//�f�v�X�X�e���V���r���[����
	HRESULT CreateDepthStencilView();
	//�t�F���X�쐬
	HRESULT CreateID3D12Fence();

	//PMD�ǂݍ���
	HRESULT LoadPMDFile(const char* path);

	//�}�e���A���f�[�^�쐬
	HRESULT CreateMaterialData();

	//�V�F�[�_�[�I�u�W�F�N�g����
	HRESULT CreateShaderObject();

	//���[�g�V�O�l�`������
	HRESULT CreateRootSig();

	//�O���t�B�b�N�X�p�C�v���C������
	HRESULT CreateBasicGraphicsPipeline();

	//���W�ϊ��p�r���[�̐���
	HRESULT CreateSceneTransformView();

	//�f�o�b�O�p
	void DebugOutputFormatString(const char* format, ...);

	//�f�o�b�O���C���[�L����
	HRESULT EnableDebugLayer();

	//�v���W�F�N�g����e�N�X�`���ւ̑��΃p�X�擾
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath);
	//�g���q�擾
	std::string GetExtension(const std::string& path);
	//�t�@�C���l�[������
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');
	//string����wstring�ւ̕ϊ�
	std::wstring GetWideStringFromString(const std::string& str);

	//�e�N�X�`���t�@�C���ǂݍ���
	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath);
	//���e�N�X�`���쐬
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateWhiteTexture();
	//���e�N�X�`���쐬
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBlackTexture();
	//�O���f�[�V�����e�N�X�`���쐬
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateGrayGradationTexture();

	//�e�N�X�`���ǂݍ��݂̃����_���e�[�u���쐬
	void CreateTextureLoaderTable();

	//�V���O���g���̂��߂ɃR���X�g���N�^��private��
	//����ɃR�s�[�Ƒ�����֎~����
	Application() {}
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;

public:
	//Application�N���X�̃V���O���g���C���X�^���X�𓾂�
	static Application& Instance();

	//������
	bool Init();

	//���[�v�N��
	void Run();

	//�㏈��
	void Terminate();

	~Application();
};