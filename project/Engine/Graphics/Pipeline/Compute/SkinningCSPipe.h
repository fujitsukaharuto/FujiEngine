#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// スキニング用パイプライン
	/// </summary>
	class SkinningCSPipe :public BasePipeline {
	public:
		SkinningCSPipe() = default;
		~SkinningCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
