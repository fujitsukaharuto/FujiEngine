#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


/// <summary>
/// 線描画用パイプライン
/// </summary>
class BaseGridPipe :public BasePipeline {
public:
	BaseGridPipe() = default;
	~BaseGridPipe();

public:



private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

private:


};