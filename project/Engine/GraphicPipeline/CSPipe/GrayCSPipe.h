#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// グレースケール用パイプライン(CS)
	/// </summary>
	class GrayCSPipe :public BasePipeline {
	public:
		GrayCSPipe() = default;
		~GrayCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
