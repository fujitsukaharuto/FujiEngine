#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクルEmit用パイプライン(CS)
	/// </summary>
	class EmitterParticleCSPipe :public BasePipeline {
	public:
		EmitterParticleCSPipe() = default;
		~EmitterParticleCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
