#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


/// <summary>
/// ガウスエフェクト用パイプライン(CS)
/// </summary>
class GaussCSPipe :public BasePipeline {
public:
	GaussCSPipe() = default;
	~GaussCSPipe();

public:



private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

private:


};
