#pragma once
#include <d3d12.h>
#include <cmath>
#include <memory>
#include "Engine/DXC/Command/CommandContext.h"
#include "Engine/DXC/FrameCount.h"

namespace DXC {

	/// <summary>
	/// コマンドの投入経路
	/// </summary>
	enum class SubmitPath {
		Frame,     //!< 毎フレームの描画。graphics と compute の両方が動く
		Immediate, //!< 初期化や単発転送。GPU完了まで待ってから戻る
	};

	/// <summary>
	/// コマンド関連のクラス
	/// </summary>
	class DXCommand {
	public:
		DXCommand() = default;
		~DXCommand();

	public:

		void Initialize(ID3D12Device* device);
		void Flush();

		/// <summary>
		/// コマンドリストをクローズし、コマンドキューに実行を送信する
		/// </summary>
		/// <param name="path">どの経路か</param>
		void Close(SubmitPath path = SubmitPath::Frame);

		/// <summary>
		/// GPU実行キューへ送信する
		/// </summary>
		/// <param name="path">どの経路か</param>
		void Execution(SubmitPath path = SubmitPath::Frame);

		/// <summary>
		/// Compute側の実行
		/// </summary>
		void ComputeExecution();

		/// <summary>
		/// シグナルを出す
		/// </summary>
		/// <param name="path">どの経路か</param>
		void GPUSignal(SubmitPath path = SubmitPath::Frame);

		/// <summary>
		/// Compute側のシグナル
		/// </summary>
		void GPUComputeSignal();

		/// <summary>
		/// GPUの実行をまつ
		/// </summary>
		/// <param name="path">どの経路か</param>
		void WaitForGPU(SubmitPath path = SubmitPath::Frame);

		/// <summary>
		/// Computeを待つ
		/// </summary>
		void WaitComputeInGraphicsQueue();

		/// <summary>
		/// Compute Queueが前フレームのGraphics完了を待つ
		/// </summary>
		/// <remarks>graphics(N)が読んでいる間に compute(N+1) が上書きするのを防ぐ</remarks>
		void WaitGraphicsInComputeQueue();

		/// <summary>
		/// コマンドアロケータおよびコマンドリストをリセットする
		/// </summary>
		/// <param name="path">どの経路か</param>
		void Reset(SubmitPath path = SubmitPath::Frame);

		void PerFrameWait();

		/// <summary>
		/// ViewとScissorをセットする
		/// </summary>
		void SetViewAndScissor(UINT width, UINT height);

		//========================================================================*/
		//* Getter
		ID3D12CommandQueue* GetQueue() const { return graphicsContext_->GetQueue(); }
		ID3D12CommandQueue* GetComputeQueue() const { return computeContext_->GetQueue(); }
		ID3D12GraphicsCommandList* GetList() const { return graphicsContext_->GetList(); }
		ID3D12GraphicsCommandList* GetComputeList() const { return computeContext_->GetList(); }
		ID3D12GraphicsCommandList* GetImmediateList() const { return immediateContext_->GetList(); }

		// レイトレ用。加速構造の構築に使う。非対応環境では nullptr
		ID3D12GraphicsCommandList4* GetList4() const { return graphicsContext_->GetList4(); }
		ID3D12GraphicsCommandList4* GetComputeList4() const { return computeContext_->GetList4(); }
		ID3D12GraphicsCommandList4* GetImmediateList4() const { return immediateContext_->GetList4(); }

		/// <summary>加速構造(BLAS/TLAS)の構築を記録するコンテキスト</summary>
		/// <remarks>割り当ては Initialize の1箇所。DIRECT か COMPUTE のいずれかであること</remarks>
		CommandContext* GetASBuildContext() const { return asBuildContext_; }

		/// <summary>加速構造の構築が描画と同じQueueに乗っているか</summary>
		/// <remarks>違うならフェンスの待ち合わせが要る。判定はここに集約すること</remarks>
		bool IsASBuildOnGraphicsQueue() const { return asBuildContext_ == graphicsContext_.get(); }

		uint32_t GetNowFrameIndex() { return frameIndex_; }

		//========================================================================*/
		//* Setter
		void SetFrameIndex(uint32_t frameIndex) { frameIndex_ = frameIndex; }

	private:


	private:

		uint32_t frameIndex_ = 0;

		D3D12_VIEWPORT viewport_{};
		D3D12_RECT scissor_{};

		std::unique_ptr<CommandContext> graphicsContext_;
		std::unique_ptr<CommandContext> computeContext_;
		std::unique_ptr<CommandContext> immediateContext_;

		// 上のいずれかを指す。所有しないので生ポインタ（WaitQueueFor と同じ流儀）
		CommandContext* asBuildContext_ = nullptr;
	};
}