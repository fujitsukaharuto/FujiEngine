#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"

namespace Graphics {

	/// <summary>
	/// 炎エフェクト用パイプライン
	/// </summary>
	class FirePipe :public BasePipeline {
	public:
		FirePipe() = default;
		~FirePipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
