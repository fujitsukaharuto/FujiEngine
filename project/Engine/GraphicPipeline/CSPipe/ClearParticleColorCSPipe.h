#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


/// <summary>
/// プール2枚化(ピンポン)用: 書き込み先プールのcolorを0クリアするパイプライン(CS)
/// </summary>
class ClearParticleColorCSPipe :public BasePipeline {
public:
	ClearParticleColorCSPipe() = default;
	~ClearParticleColorCSPipe();

private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

};
