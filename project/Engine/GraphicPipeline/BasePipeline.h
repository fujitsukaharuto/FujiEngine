#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;


class DXCom;

class BasePipeline {
public:
	BasePipeline() = default;
	~BasePipeline();

	void Initialize(DXCom* pDxcom);

	void SetPipelineState();
	void SetPipelineCSState();

	void CreatePipe();

private:

	virtual void CreateRootSignature(ID3D12Device* device);

	virtual void CreatePSO(ID3D12Device* device);

protected:

	DXCom* dxcommon_;

	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	ComPtr<ID3D12PipelineState> pso_ = nullptr;

	ComPtr<IDxcBlob> vs = nullptr;
	ComPtr<IDxcBlob> ps = nullptr;

	const std::wstring kDirectoryPath_ = L"./resource/Shaders/";

};
