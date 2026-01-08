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
		void SetCSPipeline(Pipe type, uint32_t index = 0);

		ID3D12RootSignature* GetRootSignature(Pipe type);

	private:

		template<class T>
		void CreatePipe(Pipe type);

		template<class T>
		void CreatePipe(Pipe type, std::function<void(T&)> setup);

	private:

		DXCom* dxcommon_;
		std::array<std::unique_ptr<BasePipeline>, static_cast<size_t>(Pipe::Count)> pipelines_;

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
		std::function<void(T&)> setup
	) {
		auto pipe = std::make_unique<T>();
		setup(*pipe);
		pipe->Initialize(dxcommon_);
		pipelines_[static_cast<size_t>(type)] = std::move(pipe);
	}
}