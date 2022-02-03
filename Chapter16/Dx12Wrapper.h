#pragma once
#include<d3d12.h>
#include<dxgi1_6.h>
#include<map>
#include<memory>
#include<unordered_map>
#include<DirectXTex.h>
#include<wrl.h>
#include<string>
#include<functional>
#include<array>

class PMDRenderer;

class Dx12Wrapper
{
	SIZE _winSize;
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//DXGI�܂��
	ComPtr < IDXGIFactory4> _dxgiFactory = nullptr;//DXGI�C���^�[�t�F�C�X
	ComPtr < IDXGISwapChain4> _swapchain = nullptr;//�X���b�v�`�F�C��

	//DirectX12�܂��
	ComPtr< ID3D12Device> _dev = nullptr;//�f�o�C�X
	ComPtr < ID3D12CommandAllocator> _cmdAllocator = nullptr;//�R�}���h�A���P�[�^
	ComPtr < ID3D12GraphicsCommandList> _cmdList = nullptr;//�R�}���h���X�g
	ComPtr < ID3D12CommandQueue> _cmdQueue = nullptr;//�R�}���h�L���[

	//�\���Ɋւ��o�b�t�@����
	std::vector<ID3D12Resource*> _backBuffers;//�o�b�N�o�b�t�@(2�ȏ�c�X���b�v�`�F�C�����m��)
	ComPtr<ID3D12DescriptorHeap> _rtvHeaps = nullptr;//�����_�[�^�[�Q�b�g�p�f�X�N���v�^�q�[�v
	float _bgColor[4] = { 0.5f,0.5f,0.5f,1.f };

	//�[�x�֘A
	ComPtr<ID3D12Resource> _depthBuffer = nullptr;//�[�x�o�b�t�@
	ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;//�[�x�o�b�t�@�r���[�p�f�X�N���v�^�q�[�v
	ComPtr<ID3D12DescriptorHeap> _depthSRVHeap = nullptr;
	//�V���h�E�}�b�v�p�[�x�o�b�t�@�\
	ComPtr<ID3D12Resource> _lightDepthBuffer = nullptr;
	//�e�p�p�C�v���C��
	ComPtr<ID3D12PipelineState> _shadowPipline = nullptr;                                                                                                                                                                                     

	std::unique_ptr<D3D12_VIEWPORT> _viewport;//�r���[�|�[�g
	std::unique_ptr<D3D12_RECT> _scissorrect;//�V�U�[��`

	//�V�[�����\������o�b�t�@�܂��
	ComPtr<ID3D12Resource> _sceneConstBuff = nullptr;

	struct SceneData {
		DirectX::XMMATRIX view;	//�r���[�s��
		DirectX::XMMATRIX proj;	//�v���W�F�N�V�����s��
		DirectX::XMMATRIX invProj;	//�t�v���W�F�N�V�����s��
		DirectX::XMMATRIX lightCamera;	//���C�g���猩���r���[
		DirectX::XMMATRIX shadow;	//�e
		DirectX::XMFLOAT3 eye;//���_���W
	};
	SceneData* _mappedSceneData;
	ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;
	//���s���C�g�̌���
	DirectX::XMFLOAT3 _perallelLightvVec;
	//Fov
	float _fov = DirectX::XM_PIDIV4;

	//�t�F���X
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	struct PeraVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	PeraVertex pv[4] = {
		{ {-1,-1,0.1}	,{0,1}},	//����
		{ {-1,1,0.1}	,{0,0}},	//����
		{ {1,-1,0.1}	,{1,1}},	//�E��
		{ {1,1,0.1}		,{1,0}} };	//�E��
	ComPtr<ID3D12Resource> _peraVB = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _peraVBV;

	ComPtr<ID3D12RootSignature> _peraRootSignature = nullptr;
	ComPtr<ID3D12PipelineState> _peraPipeline = nullptr;
	ComPtr<ID3D12RootSignature> _peraRootSignature2 = nullptr;
	ComPtr<ID3D12PipelineState> _peraPipeline2 = nullptr;

	//1���ڂ̃����_�����O����
	//�y���|���S��
	std::array<ComPtr<ID3D12Resource>,2> _peraResource;
	ComPtr<ID3D12Resource> _peraResource2 = nullptr;
	ComPtr<ID3D12DescriptorHeap> _peraRTVHeap = nullptr;	//�����_�[�^�[�Q�b�g�p
	ComPtr<ID3D12DescriptorHeap> _peraSRVHeap = nullptr;	//�e�N�X�`���p

	HRESULT CreatePeraResourceAndView();
	//�y���|���S���̒��_�o�b�t�@�\���r���[����
	HRESULT CreatePeraVertexBufferAndView();
	//�y���|���S���p�̃p�C�v���C������
	HRESULT CreatePeraPipeline();

	//�ŏI�I�ȃ����_�[�^�[�Q�b�g�̐���
	HRESULT	CreateFinalRenderTargets();
	//�f�v�X�X�e���V���r���[�̐���
	HRESULT CreateDepthStencilView();
	//�f�v�X�pSRV����
	HRESULT CreateDepthSRV();
	//�e�p�p�C�v���C������
	HRESULT CreateShadowPipeline();

	//�X���b�v�`�F�C���̐���
	HRESULT CreateSwapChain(const HWND& hwnd);

	//DXGI�܂�菉����
	HRESULT InitializeDXGIDevice();

	//�R�}���h�܂�菉����
	HRESULT InitializeCommand();

	//�r���[�v���W�F�N�V�����p�r���[�̐���
	HRESULT CreateSceneView();

	//���[�h�p�e�[�u��
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map < std::string, LoadLambda_t> _loadLambdaTable;
	//�e�N�X�`���e�[�u��
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> _textureTable;
	//�e�N�X�`�����[�_�e�[�u���̍쐬
	void CreateTextureLoaderTable();
	//�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[
	ID3D12Resource* CreateTextureFromFile(const char* texpath);

	//�K�E�V�A���t�B���^�֘A
	ComPtr<ID3D12Resource> _gaussianBuff = nullptr;
	std::vector<float> _gaussianWeights;
	std::vector<float> GetGaussianWeights(size_t count, float s);
	HRESULT CreateGaussianConstantBufferAndView();

	//�c�݃e�N�X�`���p
	ComPtr<ID3D12DescriptorHeap> _effectSRVHeap = nullptr;
	ComPtr<ID3D12Resource> _effectTexBuffer = nullptr;
	HRESULT CreateEffectBufferAndView();

	//�u���[��
	std::array<ComPtr<ID3D12Resource>, 2> _bloomBuffer;	//�u���[���p�o�b�t�@�[
	ComPtr<ID3D12DescriptorHeap> _bloomHeap = nullptr;
	HRESULT CreateBloomBufferAndView();
	//��ʑS�̂ڂ����p�p�C�v���C��
	ComPtr<ID3D12PipelineState> _blurPipeline;
	//��ʊE�[�x�p�ڂ����o�b�t�@�[
	ComPtr<ID3D12Resource> _dofBuffer;
	HRESULT CreateDofBuffer();

	//�A���r�G���g�I�N���[�W�����֘A
	ComPtr<ID3D12Resource> _aoBuffer = nullptr;
	ComPtr<ID3D12PipelineState> _aoPipeline = nullptr;
	ComPtr<ID3D12DescriptorHeap> _aoRTVHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> _aoSRVHeap = nullptr;
	HRESULT CreateAmbientOcclusionBuffer();
	HRESULT CreateAmbientOcclusionDescriptorHeap();

	// imgui�p�̃q�[�v����
	ComPtr<ID3D12DescriptorHeap> _heapForImgui = nullptr;	//�q�[�v�ێ��p
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeapForImgui();

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	//�V���h�E�̃����_�����O
	void PreDrawShadow();
	void EndDrawShadow();

	//1�p�X�ڂ̃����_�����O
	void PreDrawRenderTarget1();
	void DrawRenderTarget1(std::shared_ptr<PMDRenderer>& renderer);
	void EndDrawRenderTarget1();
	//2�p�X�ڂ̃����_�����O
	void PreDrawRenderTarget2();
	void DrawRenderTarget2();
	void EndDrawRenderTarget2();

	//�ŏI�����_�����O
	void PreDrawFinalRenderTarget();
	//�ŏI�����_�����O���ʕ`��
	void DrawFinalRenderTarget();
	//�k���o�b�t�@�[�ւ̏�������
	void DrawShrinkTextureForBlur();
	//�A���r�G���g�I�N���[�W�����`��
	void DrawAmbientOcculusion();

	void EndDraw();
	void BeginDraw();
	///�e�N�X�`���p�X����K�v�ȃe�N�X�`���o�b�t�@�ւ̃|�C���^��Ԃ�
	///@param texpath �e�N�X�`���t�@�C���p�X
	ComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);

	ComPtr< ID3D12Device> Device();//�f�o�C�X
	ComPtr < ID3D12GraphicsCommandList> CommandList();//�R�}���h���X�g
	ComPtr < IDXGISwapChain4> Swapchain();//�X���b�v�`�F�C��
	ComPtr<ID3D12DescriptorHeap> GetHeapForImgui();	//ImGui�p�̃q�[�v�擾
	void SetScene();
	//�V�[���ݒ�
	void SetCameraSetting();

	//Imgui�ɂ�鑀�씽�f
	void SetDebugDisplay(bool flg);	//�f�o�b�O�\����ON / OFF
	void SetSSAO(bool flg);			//SSAO��ON / OFF
	void SetSelfShadow(bool flg);	//�Z���t�V���h�E ON / OFF
	void SetFov(float fov);			//��p(30���`150��)
	void SetLightVector(float vec[3]);	//�����x�N�g��(xyz�x�N�g��)
	void SetBackColor(float col[4]);	//�w�i�F�̕ύX
	void SetBloomColor(float col[3]);	//�u���[���̐F�t��
};

