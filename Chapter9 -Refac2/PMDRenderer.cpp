#include "PMDRenderer.h"
#include "Dx12Wrapper.h"
#include <cassert>

using namespace Microsoft::WRL;

//���[�g�V�O�l�`������
HRESULT PMDRenderer::CreateRootSig()
{
	CD3DX12_DESCRIPTOR_RANGE descTblRange[4] = {};//�e�N�X�`���ƒ萔�̂Q��
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);//�萔[b0](�r���[�v���W�F�N�V�����p)
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);//�萔[b1](���[���h�A�{�[���p)
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);//�萔[b2](�}�e���A���p)
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);// �e�N�X�`��4��(��{��sph��spa�ƃg�D�[��)

	CD3DX12_ROOT_PARAMETER rootparam[3] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRange[0]);//�r���[�v���W�F�N�V�����ϊ�
	rootparam[1].InitAsDescriptorTable(1, &descTblRange[1]);//���[���h�E�{�[���ϊ�
	rootparam[2].InitAsDescriptorTable(2, &descTblRange[2]);//�}�e���A������

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = 3;//���[�g�p�����[�^��
	rootSignatureDesc.pParameters = &rootparam[0];//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumStaticSamplers = 2;
	rootSignatureDesc.pStaticSamplers = &samplerDesc[0];
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	auto result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	RETURN_FAILED(result);

	result = _dx12.Device()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootsignature.ReleaseAndGetAddressOf()));

	return result;
}

//PMD�p�̃O���t�B�b�N�X�p�C�v���C������
HRESULT PMDRenderer::CreateBasicGraphicsPipelineForPMD()
{
	//�V�F�[�_�[�I�u�W�F�N�g
	ComPtr<ID3DBlob> _vsBlob = nullptr;
	ComPtr<ID3DBlob> _psBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	auto result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//�s�V�������ȁc
	}

	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//�s�V�������ȁc
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "EDGE_FLG",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = rootsignature.Get();
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(_vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(_psBlob.Get());

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//���g��0xffffffff

	//�n���V�F�[�_�A�h���C���V�F�[�_�A�W�I���g���V�F�[�_�͐ݒ肵�Ȃ�
	gpipeline.HS.BytecodeLength = 0;
	gpipeline.HS.pShaderBytecode = nullptr;
	gpipeline.DS.BytecodeLength = 0;
	gpipeline.DS.pShaderBytecode = nullptr;
	gpipeline.GS.BytecodeLength = 0;
	gpipeline.GS.pShaderBytecode = nullptr;

	//���X�^���C�U(RS)
	//gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�

	//OutputMerger����
	//�����_�[�^�[�Q�b�g
	gpipeline.NumRenderTargets = 1;//��)���̃^�[�Q�b�g���Ɛݒ肷��t�H�[�}�b�g����
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//��v�����Ă�����

	//�[�x�X�e���V��
	gpipeline.DepthStencilState.DepthEnable = true;//�[�x
	gpipeline.DepthStencilState.StencilEnable = false;//���Ƃ�
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//�u�����h�ݒ�
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.NodeMask = 0;
	gpipeline.SampleDesc.Count = 1;
	gpipeline.SampleDesc.Quality = 0;
	gpipeline.SampleMask = 0xffffffff;//�S���Ώ�
	gpipeline.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//�ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	//�ЂƂ܂��_�����Z�͎g�p���Ȃ�
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	gpipeline.NumRenderTargets = 1;//���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

	result = _dx12.Device()->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(_pipelinestate.ReleaseAndGetAddressOf()));

	return result;
}

//���e�N�X�`������
Microsoft::WRL::ComPtr<ID3D12Resource> PMDRenderer::CreateWhiteTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> whiteBuff = nullptr;

	auto result = _dx12.Device()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	//�f�[�^�]��
	result = whiteBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<UINT>(data.size()));
	return whiteBuff;
}

//���e�N�X�`������
Microsoft::WRL::ComPtr<ID3D12Resource> PMDRenderer::CreateBlackTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> blackBuff = nullptr;

	auto result = _dx12.Device()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(blackBuff.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		return nullptr;
	}
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	//�f�[�^�]��
	result = blackBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<UINT>(data.size()));
	return blackBuff;
}

//�O���[�e�N�X�`������
Microsoft::WRL::ComPtr<ID3D12Resource> PMDRenderer::CreateGrayGradationTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	ComPtr<ID3D12Resource> gradBuff = nullptr;
	auto result = _dx12.Device()->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		return nullptr;
	}

	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + 4, col);//�f�[�^����
		--c;
	}

	result = gradBuff->WriteToSubresource(0, nullptr, data.data(),
		4 * sizeof(unsigned int), sizeof(unsigned) * static_cast<UINT>(data.size()));
	return gradBuff;
}

PMDRenderer::PMDRenderer(Dx12Wrapper& dx12) : _dx12(dx12)
{
	//���[�g�V�O�l�`������
	if (FAILED(CreateRootSig())) {
		assert(0);
	}
	//�O���t�B�b�N�X�p�C�v���C������
	if (FAILED(CreateBasicGraphicsPipelineForPMD())) {
		assert(0);
	}
	_whiteTex = CreateWhiteTexture();
	_blackTex = CreateBlackTexture();
	_gradTex = CreateGrayGradationTexture();
}

void PMDRenderer::Init()
{
}
void PMDRenderer::Update()
{
}

void PMDRenderer::Draw()
{
}

//���[�g�V�O�l�`���̃|�C���^�擾
ID3D12RootSignature* PMDRenderer::GetRootSignature()
{
	return rootsignature.Get();
}
//�O���t�B�b�N�X�p�C�v���C���̃|�C���^�擾
ID3D12PipelineState* PMDRenderer::GetPipelineState()
{
	return _pipelinestate.Get();
}
