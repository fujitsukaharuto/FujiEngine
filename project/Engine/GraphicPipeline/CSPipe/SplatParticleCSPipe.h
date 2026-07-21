#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


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
