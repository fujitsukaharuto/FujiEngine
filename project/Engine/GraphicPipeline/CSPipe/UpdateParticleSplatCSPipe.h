#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


/// <summary>
/// GPUパーティクルUpdate用パイプライン(CS) ※splatモード専用
/// 視錐台カリング/描画リスト構築を持たない軽量版。ルートシグネチャはシェーダリフレクションから生成。
/// </summary>
class UpdateParticleSplatCSPipe :public BasePipeline {
public:
	UpdateParticleSplatCSPipe() = default;
	~UpdateParticleSplatCSPipe();

private:

	void CreateRootSignature(ID3D12Device* device)override;

	void CreatePSO(ID3D12Device* device)override;

};
