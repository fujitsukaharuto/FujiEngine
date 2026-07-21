#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// GPUパーティクルUpdate用パイプライン(CS) ※splatモード専用
	/// 視錐台カリング/描画リスト構築を持たない軽量版。ルートシグネチャはシェーダリフレクションから生成。
	/// </summary>
	class UpdateParticleSplatCSPipe :public BasePipeline {
	public:
		UpdateParticleSplatCSPipe() = default;
		~UpdateParticleSplatCSPipe();

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	};

}
