#pragma once
#include <string>
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// シェーダパスだけが異なる標準的なCS用パイプライン
	/// </summary>
	/// <remarks>Outline と TrailEmit は個別クラスのまま</remarks>
	class ComputePipeline : public BasePipeline {
	public:
		ComputePipeline() = default;
		~ComputePipeline();

		/// <summary>CS の HLSL パス(kDirectoryPath_ からの相対)を Initialize より前に設定する</summary>
		void SetShaderPath(const std::wstring& path) { csPath_ = path; }

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

	private:

		std::wstring csPath_;

	};

}
