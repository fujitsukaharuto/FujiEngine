#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// ランダムエフェクト用パイプライン(CS)
	/// </summary>
	class RandomCSPipe :public BasePipeline {
	public:
		RandomCSPipe() = default;
		~RandomCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
