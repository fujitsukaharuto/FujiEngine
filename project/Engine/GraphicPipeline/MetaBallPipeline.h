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
	/// メタボールエフェクト用パイプライン
	/// </summary>
	class MetaBallPipeline :public BasePipeline {
	public:
		MetaBallPipeline() = default;
		~MetaBallPipeline();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
