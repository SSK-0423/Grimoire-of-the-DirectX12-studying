#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <d3dcompiler.h>

#include <wrl.h>

#include <vector>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

#define RETURN_FAILED(result) if(FAILED(result)){ assert(0); return result; }

class Dx12Wrapper;
class PMDActor;

class PMDRenderer {
	friend PMDActor;
private:
	Dx12Wrapper& _dx12;

	//���[�g�V�O�l�`�����O���t�B�b�N�X�p�C�v���C��
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootsignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;

	//�f�t�H���g�e�N�X�`��
	Microsoft::WRL::ComPtr<ID3D12Resource> _whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _gradTex = nullptr;

	//���[�g�V�O�l�`������
	HRESULT CreateRootSig();
	//�O���t�B�b�N�X�p�C�v���C������
	HRESULT CreateBasicGraphicsPipelineForPMD();

	//Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultTexture(size_t width, size_t height);
	//���e�N�X�`���쐬
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateWhiteTexture();
	//���e�N�X�`���쐬
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBlackTexture();
	//�O���f�[�V�����e�N�X�`���쐬
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateGrayGradationTexture();

public:
	PMDRenderer(Dx12Wrapper& dx12);
	void Init();
	void Update();
	void Draw();

	ID3D12RootSignature* GetRootSignature();
	ID3D12PipelineState* GetPipelineState();
};