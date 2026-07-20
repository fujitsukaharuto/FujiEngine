#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include "Engine/GraphicPipeline/BasePipeline.h"

using namespace Microsoft::WRL;

namespace Graphics {

	/// <summary>
	/// 炎エフェクト用パイプライン
	/// </summary>
	class FirePipe :public BasePipeline {
	public:
		FirePipe() = default;
		~FirePipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
