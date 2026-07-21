#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// CRTエフェクト用パイプライン
	/// </summary>
	class CRTPipe :public BasePipeline {
	public:
		CRTPipe() = default;
		~CRTPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
