#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "BasePipeline.h"

using namespace Microsoft::WRL;


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
