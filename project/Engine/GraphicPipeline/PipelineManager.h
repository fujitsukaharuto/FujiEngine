#pragma once
#include "BasePipeline.h"
#include "PipeKind.h"
#include <memory>
#include <array>
#include <functional>

class DXCom;

namespace Graphics {
	/// <summary>
	/// パイプライン管理クラス
	/// </summary>
	class PipelineManager {
	public:
		PipelineManager() = default;
		~PipelineManager();

	public:

		static PipelineManager* GetInstance();

		void Initialize(DXCom* pDxcom);
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
		void SetGraphicsRootCBV(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address);
		void SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_DESCRIPTOR_HANDLE handle);
		
		void SetComputeRootCBV(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address);
		void SetComputeRootDescriptorTable(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_DESCRIPTOR_HANDLE handle);
		void SetComputeRoot32BitConstants(ID3D12GraphicsCommandList* list, const std::string& name, UINT numValues, const void* data, UINT offset);

	private:

		template<class T>
		void CreatePipe(Pipe type);

		template<class T>
		void CreatePipe(Pipe type, const std::function<void(T&)>& setup);

	private:

		DXCom* dxcommon_;
		std::array<std::unique_ptr<BasePipeline>, static_cast<size_t>(Pipe::Count)> pipelines_;

		BasePipeline* currentPipeline_ = nullptr;

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