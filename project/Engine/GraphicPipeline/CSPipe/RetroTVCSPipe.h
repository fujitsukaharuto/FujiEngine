#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// レトロテレビエフェクト用パイプライン(CS)
	/// </summary>
	class RetroTVCSPipe :public BasePipeline {
	public:
		RetroTVCSPipe() = default;
		~RetroTVCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
