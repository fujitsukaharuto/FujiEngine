#pragma once
#include <string>
#include <d3d12.h>
#include "Engine/GraphicPipeline/BasePipeline.h"


namespace Graphics {

	/// <summary>
	/// シェーダパスだけが異なる標準的なCS用パイプライン。
	/// リフレクション駆動でルートシグネチャを生成するCSはこの1クラスに集約する
	/// (静的サンプラ付きの Outline や手書きRootSigの TrailEmit は個別クラスのまま)。
	/// </summary>
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
