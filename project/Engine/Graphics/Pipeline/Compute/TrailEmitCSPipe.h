#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクルUpdate用パイプライン(CS)
	/// </summary>
	class TrailEmitCSPipe :public BasePipeline {
	public:
		TrailEmitCSPipe() = default;
		~TrailEmitCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
