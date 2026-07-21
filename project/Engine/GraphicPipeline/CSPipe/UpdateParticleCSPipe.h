#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクルUpdate用パイプライン(CS)
	/// </summary>
	class UpdateParticleCSPipe :public BasePipeline {
	public:
		UpdateParticleCSPipe() = default;
		~UpdateParticleCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
