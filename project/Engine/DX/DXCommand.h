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
		void Close(uint32_t index = 0);

		/// <summary>
		/// GPU実行キューへ送信する
		/// </summary>
		void Execution(uint32_t index = 0);

		void GPUSignal(uint32_t index = 0);

		/// <summary>
		/// GPUの実行をまつ
		/// </summary>
		void WaitForGPU(uint32_t index = 0);

		/// <summary>
		/// コマンドアロケータおよびコマンドリストをリセットする
		/// </summary>
		void Reset(uint32_t index = 0);

		/// <summary>
		/// ViewとScissorをセットする
		/// </summary>
		void SetViewAndscissor();

		//========================================================================*/
		//* Getter
		ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
		ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }
		ID3D12GraphicsCommandList* GetImmediateList() const { return immediateList_.Get(); }
		uint32_t GetNowFrameIndex() { return frameIndex_; }

		void SetFrameIndex(uint32_t frameIndex) { frameIndex_ = frameIndex; }

	private:


	private:

		ComPtr<ID3D12CommandQueue> queue_ = nullptr;
		ComPtr<ID3D12CommandAllocator> allocator_[kFrameCount_];
		ComPtr<ID3D12GraphicsCommandList> list_ = nullptr;

		ComPtr<ID3D12Fence> fence_ = nullptr;
		uint64_t fenceValue_[kFrameCount_];
		uint64_t globalFenceValue_ = 0;

		uint32_t frameIndex_ = 0;

		D3D12_VIEWPORT viewport_{};
		D3D12_RECT scissor_{};


		ComPtr<ID3D12CommandAllocator> immediateAllocator_;
		ComPtr<ID3D12GraphicsCommandList> immediateList_;

		ComPtr<ID3D12Fence> immediateFence_;
		uint64_t immediateFenceValue_ = 0;
	};
}