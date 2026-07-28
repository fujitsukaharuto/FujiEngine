#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// アニメーション用パイプライン
	/// </summary>
	class AnimationPipeline :public BasePipeline {
	public:
		AnimationPipeline() = default;
		~AnimationPipeline();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
