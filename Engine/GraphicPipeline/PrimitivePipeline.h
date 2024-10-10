#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;



class PrimitivePipeline :public BasePipeline{
public:
	PrimitivePipeline() = default;
	~PrimitivePipeline() = default;

	/// <summary>
	/// psoの取得
	/// </summary>
	/// <returns></returns>
	const ComPtr<ID3D12PipelineState>& GetPipelineState()const{ return pso_; }

	/// <summary>
	/// rootSignatureの取得
	/// </summary>
	/// <returns></returns>
	const ComPtr<ID3D12RootSignature>& GetRootSignature()const{ return rootSignature_; }
private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

};
