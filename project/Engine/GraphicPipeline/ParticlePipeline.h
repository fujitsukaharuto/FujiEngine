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
	/// パーティクル用パイプライン
	/// </summary>
	class ParticlePipeline :public BasePipeline {
	public:
		ParticlePipeline() = default;
		~ParticlePipeline();

	public:

		/// <summary>
		/// 減算合成にするかどうか
		/// </summary>
		void SetBlendType(BlendType type) { type_ = type; }

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:

		BlendType type_ = BlendType::ADD;

	};

}
