#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;



class LuminanceOutlineCSPipe :public BasePipeline {
public:
	LuminanceOutlineCSPipe() = default;
	~LuminanceOutlineCSPipe();

public:



private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

private:


};
