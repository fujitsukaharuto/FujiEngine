#pragma once
#include "Engine/Graphics/Pipeline/BasePipeline.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/ObjectShaderFeatures.h"
#include <memory>
#include <array>
#include <functional>
#include <future>
#include <unordered_map>
#include <vector>

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

		/// <summary>今のパイプラインの、PS をマクロ違いでコンパイルした PSO へ切り替える (無ければ何もしない)</summary>
		/// <remarks>ルートシグネチャが同じなので結線は消えない。描き終えたら false で戻すこと</remarks>
		void SetPipelineVariant(bool isVariant);

		/// <summary>前方描画の PS を、この機能 (ObjectShaderFeature の組み合わせ) だけでコンパイルしたものへ切り替える</summary>
		/// <remarks>初めての組み合わせは裏でコンパイルし、出来るまでは全部入りで描く。描く前に毎フレーム呼ぶ</remarks>
		void SetObjectFeatures(uint32_t features);
		/// <summary>今張っている前方描画の PS の機能。全部入りなら kObjectFeatureAll</summary>
		uint32_t GetActiveObjectFeatures() const { return activeObjectFeatures_; }
		/// <summary>裏でコンパイルしている組み合わせがあるか</summary>
		bool IsCompilingObjectFeatures() const;
		/// <summary>false にすると常に全部入りで描く。比べる用</summary>
		void SetObjectFeaturesEnabled(bool enabled) { isObjectFeaturesEnabled_ = enabled; }
		bool IsObjectFeaturesEnabled() const { return isObjectFeaturesEnabled_; }

		ID3D12RootSignature* GetRootSignature(Pipe type);

		BasePipeline* GetCurrentPipeline() const { return currentPipeline_; }

		// --- 名前でルートパラメータをセットするヘルパー ---
		// 名前は RootNames.h の定数(RootParam)で渡す
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

		/// <summary>積まれたパイプラインの Initialize をまとめて並列に走らせる</summary>
		/// <remarks>中身は HLSL のコンパイルが大半。各パイプラインは互いを参照しないので並列に回せる</remarks>
		void InitializePending();

		/// <summary>前方描画の PS を features でコンパイルし、objectPipes_ の並びで PSO を作る。別スレッドで走る</summary>
		/// <returns>失敗したら空</returns>
		std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> BuildObjectFeaturePSOs(uint32_t features) const;

	private:

		/// <summary>
		/// 前方描画の PS の機能の組み合わせ1つぶんの PSO
		/// </summary>
		struct ObjectFeatureVariant {
			std::future<std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>>> job;
			// objectPipes_ と同じ並び。GPU が使い終えたか分からないので終了まで消さない
			std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> psos;
			bool isDone = false;
		};

		DXC::DXCom* dxcommon_;
		std::array<std::unique_ptr<BasePipeline>, static_cast<size_t>(Pipe::Count)> pipelines_;

		// hasObjectFeatures の付いたパイプ
		std::vector<Pipe> objectPipes_;
		std::unordered_map<uint32_t, ObjectFeatureVariant> objectVariants_;
		uint32_t activeObjectFeatures_ = kObjectFeatureAll;
		bool isObjectFeaturesEnabled_ = true;

		/// <summary>生成済みでまだ Initialize していないもの。InitializePending() で消化する</summary>
		std::vector<Pipe> pendingInit_;

		BasePipeline* currentPipeline_ = nullptr;
		Pipe currentPipeType_ = Pipe::Count;	// RootParam のキャッシュを引くためのキー

	};


	template<class T>
	void PipelineManager::CreatePipe(Pipe type) {
		pipelines_[static_cast<size_t>(type)] = std::make_unique<T>();
		pendingInit_.push_back(type);
	}

	template<class T>
	void PipelineManager::CreatePipe(
		Pipe type,
		const std::function<void(T&)>& setup
	) {
		auto pipe = std::make_unique<T>();
		setup(*pipe);
		pipelines_[static_cast<size_t>(type)] = std::move(pipe);
		pendingInit_.push_back(type);
	}
}