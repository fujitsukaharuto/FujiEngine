#pragma once
#include "BasePipeline.h"
#include "PipeKind.h"
#include <memory>
#include <vector>


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


	private:


	private:

		DXCom* dxcommon_;
		std::vector<std::unique_ptr<BasePipeline>> pipelines_;


	};
}