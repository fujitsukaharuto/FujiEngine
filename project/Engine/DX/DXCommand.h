#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <cmath>

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

		/// <summary>
		/// コマンドリストをクローズし、コマンドキューに実行を送信する
		/// </summary>
		void Close();

		/// <summary>
		/// GPU実行キューへ送信する
		/// </summary>
		void Execution();

		/// <summary>
		/// コマンドアロケータおよびコマンドリストをリセットする
		/// </summary>
		void Reset(uint32_t frameIndex);

		/// <summary>
		/// ViewとScissorをセットする
		/// </summary>
		void SetViewAndscissor();

		//========================================================================*/
		//* Getter
		ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }
		ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }


	private:

		static constexpr uint32_t kFrameCount = 2;

	private:

		ComPtr<ID3D12CommandQueue> queue_ = nullptr;
		ComPtr<ID3D12CommandAllocator> allocator_[kFrameCount];
		ComPtr<ID3D12GraphicsCommandList> list_ = nullptr;

		ComPtr<ID3D12Fence> fence_ = nullptr;
		uint64_t fenceValue_ = 0;

		uint32_t frameIndex_ = 0;

		D3D12_VIEWPORT viewport_{};
		D3D12_RECT scissor_{};

	};
}