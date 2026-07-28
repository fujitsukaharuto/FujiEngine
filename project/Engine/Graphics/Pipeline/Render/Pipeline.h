#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 通常描画用パイプライン
	/// </summary>
	class Pipeline :public BasePipeline {
	public:
		Pipeline() = default;
		~Pipeline();

	public:

		/// <summary>
		/// 減算合成にするかどうか
		/// </summary>
		void SetIsAddMode(bool is) { isAddMode_ = is; }

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:

		bool isAddMode_ = false;

	};

}
