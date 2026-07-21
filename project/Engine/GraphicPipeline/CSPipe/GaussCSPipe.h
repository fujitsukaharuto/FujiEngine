#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// ガウスエフェクト用パイプライン(CS)
	/// </summary>
	class GaussCSPipe :public BasePipeline {
	public:
		GaussCSPipe() = default;
		~GaussCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
