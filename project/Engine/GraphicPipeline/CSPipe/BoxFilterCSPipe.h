#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// ボックスフィルタ用パイプライン(CS)
	/// </summary>
	class BoxFilterCSPipe :public BasePipeline {
	public:
		BoxFilterCSPipe() = default;
		~BoxFilterCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
