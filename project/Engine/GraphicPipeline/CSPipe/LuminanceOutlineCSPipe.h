#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 輝度ベースアウトライン用パイプライン(CS)
	/// </summary>
	class LuminanceOutlineCSPipe :public BasePipeline {
	public:
		LuminanceOutlineCSPipe() = default;
		~LuminanceOutlineCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
