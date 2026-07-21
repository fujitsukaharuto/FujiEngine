#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"

namespace Graphics {

	/// <summary>
	/// 雷エフェクト用パイプライン
	/// </summary>
	class ThunderPipe :public BasePipeline {
	public:
		ThunderPipe() = default;
		~ThunderPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
