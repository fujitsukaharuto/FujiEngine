#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <cmath>
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
		/// コマンドアロケータおよびコマンドリストをリセットする
		/// </summary>
		/// <param name="index">どのコマンドリストか</param>
		void Reset(uint32_t index = 0);

		void PerFrameWait();

		/// <summary>
		/// ViewとScissorをセットする
		/// </summary>
		void SetViewAndScissor();

		//========================================================================*/
		//* Getter
		ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
		ID3D12CommandQueue* GetComputeQueue() const { return computeQueue_.Get(); }
		ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }
		ID3D12GraphicsCommandList* GetComputeList() const { return computeList_.Get(); }
		ID3D12GraphicsCommandList* GetImmediateList() const { return immediateList_.Get(); }
		uint32_t GetNowFrameIndex() { return frameIndex_; }

		void SetFrameIndex(uint32_t frameIndex) { frameIndex_ = frameIndex; }

	private:

		// Queueの初期化
		void InitDefaultQueue(ID3D12Device* device);
		void InitComputeQueue(ID3D12Device* device);
		void InitImmediateQueue(ID3D12Device* device);

	private:

		ComPtr<ID3D12CommandQueue> queue_ = nullptr;
		ComPtr<ID3D12CommandAllocator> allocator_[kFrameCount_];
		ComPtr<ID3D12GraphicsCommandList> list_ = nullptr;

		ComPtr<ID3D12Fence> fence_ = nullptr;
		uint64_t fenceValue_[kFrameCount_];

		ComPtr<ID3D12CommandQueue> computeQueue_ = nullptr;
		ComPtr<ID3D12CommandAllocator> computeAllocator_[kFrameCount_];
		ComPtr<ID3D12GraphicsCommandList> computeList_ = nullptr;

		ComPtr<ID3D12Fence> computeFence_ = nullptr;
		uint64_t computeFenceValue_[kFrameCount_];


		uint64_t globalFenceValue_ = 0;
		uint64_t globalComputeFenceValue_ = 0;
		uint32_t frameIndex_ = 0;

		D3D12_VIEWPORT viewport_{};
		D3D12_RECT scissor_{};


		ComPtr<ID3D12CommandAllocator> immediateAllocator_;
		ComPtr<ID3D12GraphicsCommandList> immediateList_;

		ComPtr<ID3D12Fence> immediateFence_;
		uint64_t immediateFenceValue_ = 0;
	};
}