#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// ラジアルブラー用パイプライン(CS)
	/// </summary>
	class RadialBlurCSPipe :public BasePipeline {
	public:
		RadialBlurCSPipe() = default;
		~RadialBlurCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
