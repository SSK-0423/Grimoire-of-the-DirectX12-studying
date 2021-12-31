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

	//ルートシグネチャ＆グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootsignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;

	//デフォルトテクスチャ
	Microsoft::WRL::ComPtr<ID3D12Resource> _whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _gradTex = nullptr;

	//ルートシグネチャ生成
	HRESULT CreateRootSig();
	//グラフィックスパイプライン生成
	HRESULT CreateBasicGraphicsPipelineForPMD();

	//Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultTexture(size_t width, size_t height);
	//白テクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateWhiteTexture();
	//黒テクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBlackTexture();
	//グラデーションテクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateGrayGradationTexture();

public:
	PMDRenderer(Dx12Wrapper& dx12);
	void Init();
	void Update();
	void Draw();

	ID3D12RootSignature* GetRootSignature();
	ID3D12PipelineState* GetPipelineState();
};