#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクルTextureEmit用パイプライン(CS)
	/// </summary>
	class EmitterTexParticleCSPipe :public BasePipeline {
	public:
		EmitterTexParticleCSPipe() = default;
		~EmitterTexParticleCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
