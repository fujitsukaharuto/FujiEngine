#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cassert>
#include "Engine/DXC/FrameCount.h"

namespace DXC {

	/// <summary>
	/// コマンドキュー、アロケータ、コマンドリスト、フェンスを1組にまとめたクラス
	/// </summary>
	class CommandContext {
	public:
		CommandContext() = default;
		~CommandContext();

		// typeにD3D12_COMMAND_LIST_TYPE_DIRECTやCOMPUTEを渡して初期化を共通化
		void Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

		// コマンドの実行と同期
		void Close();
		void Execute();
		void Signal(uint32_t frameIndex);
		void WaitForGPU(uint32_t frameIndex, bool isLogging = false);
		void WaitQueueFor(CommandContext* otherContext, uint32_t frameIndex);

		//========================================================================*/
		//* Getter
		ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
		ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }

		/// <summary>
		/// レイトレ用のコマンドリスト。非対応環境では nullptr
		/// </summary>
		ID3D12GraphicsCommandList4* GetList4() const { return list4_.Get(); }

		ID3D12CommandAllocator* GetAllocator(uint32_t frameIndex) const { return allocator_[frameIndex].Get(); }
		ID3D12Fence* GetFence() const { return fence_.Get(); }
		uint64_t GetFenceValue(uint32_t frameIndex) const { return fenceValue_[frameIndex]; }

		// 次のフレームに向けてアロケータとリストをリセットする
		void Reset(uint32_t frameIndex);

		bool IsOpen() const { return isOpen_; }

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> list4_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
		uint64_t globalFenceValue_ = 0;
		uint64_t fenceValue_[DXC::kFrameCount_] = { 0 };
		HANDLE fenceEvent_ = nullptr;

		bool isOpen_ = false;
	};
}