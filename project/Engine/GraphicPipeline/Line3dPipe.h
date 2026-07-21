#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 線描画用パイプライン
	/// </summary>
	class Line3dPipe :public BasePipeline {
	public:
		Line3dPipe() = default;
		~Line3dPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
