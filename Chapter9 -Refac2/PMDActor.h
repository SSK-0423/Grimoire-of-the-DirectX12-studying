#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>

#include <d3dx12.h>
#pragma comment(lib,"d3d12.lib")

class Dx12Wrapper;
class PMDRenderer;

class PMDActor {
private:
	Dx12Wrapper& _dx12;
	PMDRenderer& _pmdRenderer;

	//�o�b�t�@�\
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;		//���_�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;		//�C���f�b�N�X�o�b�t�@

	//�r���[
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};	//���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW _ibView = {};	//�C���f�b�N�X�o�b�t�@�r���[
	
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformMat = nullptr;//���W�ϊ��s��(���̓��[���h�̂�)
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;//���W�ϊ��q�[�v
	
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
	
	//���W
	struct Transform
	{
		//�����Ɏ����Ă���XMMATRIX�����o�[��16�o�C�g�A���C�����g�ł��邽��
		//Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	Transform _transform;
	Transform* _mappedTransform = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformBuff = nullptr;

	//�}�e���A���֘A
	std::vector<Material> _materials;
	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;

	//�ǂݍ��񂾃}�e���A�������ƂɃ}�e���A���o�b�t�@���쐬
	HRESULT CreateMaterialData();
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialHeap = nullptr;
	//�}�e���A�����e�N�X�`���̂т�[���쐬
	HRESULT CreateMaterialAndTextureView();

	//���W�ϊ��p�r���[�̐���
	HRESULT CreateTransformView();

	//PMD�ǂݍ���
	HRESULT LoadPMDFile(const char* path);

	float _angle;//�e�X�g�pY����]

public:
	PMDActor(const char* path, PMDRenderer& pmdRenderer, Dx12Wrapper& dx12);
	~PMDActor();
	//�N���[���͒��_����у}�e���A���͋��ʂ̃o�b�t�@������悤�ɂ���
	PMDActor* Clone();
	void Init();
	void Update();
	void Draw();
};