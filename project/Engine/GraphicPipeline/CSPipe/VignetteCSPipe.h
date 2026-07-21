#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// ビネット用パイプライン(CS)
	/// </summary>
	class VignetteCSPipe :public BasePipeline {
	public:
		VignetteCSPipe() = default;
		~VignetteCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
