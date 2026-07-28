#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 何もオフスクリーンかけない用パイプライン
	/// </summary>
	class NonePipeline :public BasePipeline {
	public:
		NonePipeline() = default;
		~NonePipeline();

	public:

		// trueにすると加算合成(Src ONE / Dest ONE)になる。GPUパーティクルRTの合成用。
		void SetIsAddMode(bool isAdd) { isAddMode_ = isAdd; }

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:

		bool isAddMode_ = false;

	};

}
