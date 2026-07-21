#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// CRTエフェクト用パイプライン(CS)
	/// </summary>
	class CRTCSPipe :public BasePipeline {
	public:
		CRTCSPipe() = default;
		~CRTCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
