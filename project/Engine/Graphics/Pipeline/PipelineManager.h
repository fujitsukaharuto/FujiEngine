#pragma once
#include "Engine/Graphics/Pipeline/BasePipeline.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include <memory>
#include <array>
#include <functional>

namespace DXC { class DXCom; }

namespace Graphics {

	class RootParam;
	struct RenderPipelineDesc;

	/// <summary>
	/// パイプライン管理クラス
	/// </summary>
	class PipelineManager {
	public:
		PipelineManager() = default;
		~PipelineManager();

	public:

		static PipelineManager* GetInstance();

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>
		/// パイプラインを作成
		/// </summary>
		void CreatePipeline();

		/// <summary>
		/// グラフィックスパイプラインのセット
		/// </summary>
		void SetPipeline(Pipe type);

		/// <summary>
		/// コンピュートシェーダ用のパイプラインのセット
		/// </summary>
		/// <param name="type"> パイプラインのタイプ </param>
		/// <param name="index"> 0=Normal/1=Immediate/2=Compute </param>
		void SetCSPipeline(Pipe type, uint32_t index = 0);

		ID3D12RootSignature* GetRootSignature(Pipe type);

		BasePipeline* GetCurrentPipeline() const { return currentPipeline_; }

		// --- 名前でルートパラメータをセットするヘルパー ---
		// 名前は RootNames.h の定数(RootParam)で渡す。RootParam がパイプライン毎の
		// 解決結果を持つので、毎フレームの文字列ハッシュ検索は発生しない
		void SetGraphicsRootCBV(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_VIRTUAL_ADDRESS address);
		void SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_DESCRIPTOR_HANDLE handle);

		void SetComputeRootCBV(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_VIRTUAL_ADDRESS address);
		void SetComputeRootDescriptorTable(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_DESCRIPTOR_HANDLE handle);
		void SetComputeRoot32BitConstants(ID3D12GraphicsCommandList* list, const RootParam& param, UINT numValues, const void* data, UINT offset);

	private:

		template<class T>
		void CreatePipe(Pipe type);

		template<class T>
		void CreatePipe(Pipe type, const std::function<void(T&)>& setup);

		/// <summary>差分が Desc で表現できる描画パイプ用: RenderPipeline を生成して登録する</summary>
		void CreateRenderPipe(Pipe type, const RenderPipelineDesc& desc);

		/// <summary>シェーダパスだけが違う標準CS用: ComputePipeline を生成して登録する</summary>
		void CreateComputePipe(Pipe type, const std::wstring& csPath);

	private:

		DXC::DXCom* dxcommon_;
		std::array<std::unique_ptr<BasePipeline>, static_cast<size_t>(Pipe::Count)> pipelines_;

		BasePipeline* currentPipeline_ = nullptr;
		Pipe currentPipeType_ = Pipe::Count;	// RootParam のキャッシュを引くためのキー

	};


	template<class T>
	void PipelineManager::CreatePipe(Pipe type) {
		auto pipe = std::make_unique<T>();
		pipe->Initialize(dxcommon_);
		pipelines_[static_cast<size_t>(type)] = std::move(pipe);
	}

	template<class T>
	void PipelineManager::CreatePipe(
		Pipe type,
		const std::function<void(T&)>& setup
	) {
		auto pipe = std::make_unique<T>();
		setup(*pipe);
		pipe->Initialize(dxcommon_);
		pipelines_[static_cast<size_t>(type)] = std::move(pipe);
	}
}