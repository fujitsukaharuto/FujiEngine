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
