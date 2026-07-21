#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// メタボールエフェクト用パイプライン
	/// </summary>
	class MetaBallPipeline :public BasePipeline {
	public:
		MetaBallPipeline() = default;
		~MetaBallPipeline();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
