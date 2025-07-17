#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;



class SpritePipe :public BasePipeline {
public:
	SpritePipe() = default;
	~SpritePipe();

public:



private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

private:



};
