#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// スプライト描画用パイプライン
	/// </summary>
	class SpritePipe :public BasePipeline {
	public:
		SpritePipe() = default;
		~SpritePipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:



	};

}
