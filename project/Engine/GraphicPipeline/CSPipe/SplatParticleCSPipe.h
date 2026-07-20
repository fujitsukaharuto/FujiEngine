#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


namespace Graphics {

	/// <summary>
	/// パーティクルを画面へ点描(atomic加算)するスプラット用パイプライン(CS)
	/// </summary>
	class SplatParticleCSPipe :public BasePipeline {
	public:
		SplatParticleCSPipe() = default;
		~SplatParticleCSPipe();

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	};

}
