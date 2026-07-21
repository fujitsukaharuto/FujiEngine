#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクル表面Emit用パイプライン(CS)
	/// </summary>
	class EmitterSurfaceParticleCSPipe :public BasePipeline {
	public:
		EmitterSurfaceParticleCSPipe() = default;
		~EmitterSurfaceParticleCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
