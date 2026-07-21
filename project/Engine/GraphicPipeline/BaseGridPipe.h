#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 線描画用パイプライン
	/// </summary>
	class BaseGridPipe :public BasePipeline {
	public:
		BaseGridPipe() = default;
		~BaseGridPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
