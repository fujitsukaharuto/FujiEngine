#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// スプラット蓄積バッファのクリア用パイプライン(CS)
	/// </summary>
	class SplatClearCSPipe :public BasePipeline {
	public:
		SplatClearCSPipe() = default;
		~SplatClearCSPipe();

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	};

}
