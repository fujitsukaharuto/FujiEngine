#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <cmath>

using namespace Microsoft::WRL;



class DXCommand {
public:
	DXCommand() = default;
	~DXCommand();

public:

	void Initialize(ID3D12Device* device);

	void Close();

	void Execution();

	void Reset();

	void SetViewAndscissor();

	/// ==========================================
	/// Getter
	/// ==========================================

	ID3D12CommandQueue* GetQueue() const { return queue_.Get(); }

	ID3D12GraphicsCommandList* GetList() const { return list_.Get(); }


private:



private:

	ComPtr<ID3D12CommandQueue> queue_ = nullptr;
	ComPtr<ID3D12CommandAllocator> allocator_ = nullptr;
	ComPtr<ID3D12GraphicsCommandList> list_ = nullptr;

	ComPtr<ID3D12Fence> fence_ = nullptr;
	uint64_t fenceValue_ = 0;

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissor_{};

};
