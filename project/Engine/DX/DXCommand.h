#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <cmath>
#include <memory>
#include "Engine/DX/CommandContext.h"
#include "Engine/DX/FrameCount.h"

using namespace Microsoft::WRL;

namespace DXC {
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
		/// <param name="index">どのコマンドリストか</param>
		void Close(uint32_t index = 0);

		/// <summary>
		/// GPU実行キューへ送信する
		/// </summary>
		/// <param name="index">どのコマンドリストか</param>
		void Execution(uint32_t index = 0);

		/// <summary>
		/// Compute側の実行
		/// </summary>
		void ComputeExecution();

		/// <summary>
		/// シグナルを出す
		/// </summary>
		/// <param name="index">どのコマンドリストか</param>
		void GPUSignal(uint32_t index = 0);

		/// <summary>
		/// Compute側のシグナル
		/// </summary>
		void GPUComputeSignal();

		/// <summary>
		/// GPUの実行をまつ
		/// </summary>
		/// <param name="index">どのコマンドリストか</param>
		void WaitForGPU(uint32_t index = 0);

		/// <summary>
		/// Computeを待つ
		/// </summary>
		void WaitComputeInGraphicsQueue();

		/// <summary>
		/// Compute Queueが前フレームのGraphics完了を待つ。
		/// パーティクルを、graphics(N)の描画読み取り中にcompute(N+1)の更新が上書きを防ぐ。
		/// </summary>
		void WaitGraphicsInComputeQueue();

		/// <summary>
		/// コマンドアロケータおよびコマンドリストをリセットする
		/// </summary>
		/// <param name="index">どのコマンドリストか</param>
		void Reset(uint32_t index = 0);

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
	};
}