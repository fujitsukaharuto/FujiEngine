#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// スプラット蓄積バッファを加算合成でシーンへ重ねるパイプライン(フルスクリーン)
	/// </summary>
	class SplatCompositePipe :public BasePipeline {
	public:
		SplatCompositePipe() = default;
		~SplatCompositePipe();

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	};

}
