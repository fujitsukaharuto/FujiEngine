#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 衝撃波エフェクト用パイプライン
	/// </summary>
	class ShockWavePipe :public BasePipeline {
	public:
		ShockWavePipe() = default;
		~ShockWavePipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
