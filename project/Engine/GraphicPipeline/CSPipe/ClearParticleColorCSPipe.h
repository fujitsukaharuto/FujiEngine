#pragma once
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// プール2枚化(ピンポン)用: 書き込み先プールのcolorを0クリアするパイプライン(CS)
	/// </summary>
	class ClearParticleColorCSPipe :public BasePipeline {
	public:
		ClearParticleColorCSPipe() = default;
		~ClearParticleColorCSPipe();

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	};

}
