#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// ブルームエフェクト用パイプライン(CS)
	/// </summary>
	class BloomCSPipe :public BasePipeline {
	public:
		BloomCSPipe() = default;
		~BloomCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
