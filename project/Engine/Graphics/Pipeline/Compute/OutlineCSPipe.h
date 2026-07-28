#pragma once
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// 深度ベースアウトライン用パイプライン(CS)
	/// </summary>
	class OutlineCSPipe :public BasePipeline {
	public:
		OutlineCSPipe() = default;
		~OutlineCSPipe();

	public:



	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:


	};

}
