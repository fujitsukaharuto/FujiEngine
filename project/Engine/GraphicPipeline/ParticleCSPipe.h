#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


/// <summary>
/// GPUパーティクル用パイプライン
/// </summary>
class ParticleCSPipe :public BasePipeline {
public:
	ParticleCSPipe() = default;
	~ParticleCSPipe();

public:

	/// <summary>
	/// 減算合成にするかどうか
	/// </summary>
	void SetIsSubMode(bool is) { isSubMode_ = is; }

private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

private:

	bool isSubMode_ = false;

};
