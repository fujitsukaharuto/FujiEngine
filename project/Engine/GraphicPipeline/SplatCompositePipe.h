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
