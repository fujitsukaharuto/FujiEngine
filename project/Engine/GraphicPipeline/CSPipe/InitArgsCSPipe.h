#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクル初期化用パイプライン(CS)
	/// </summary>
	class InitArgsCSPipe :public BasePipeline {
	public:
		InitArgsCSPipe() = default;
		~InitArgsCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
