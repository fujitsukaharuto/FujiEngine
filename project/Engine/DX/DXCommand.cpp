#include "DXCommand.h"
#include "MyWindow.h"
#include <cassert>

using namespace DXC;
using namespace Core;


DXCommand::~DXCommand() {

	list_.Reset();
	for (uint32_t i = 0; i < kFrameCount_; ++i) {
		allocator_[i].Reset();
	}
	fence_.Reset();
	queue_.Reset();

}


void DXCommand::Initialize(ID3D12Device* device) {

	HRESULT hr;

	/// ------------------------------------------
	/// queue-------------------------------------
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	hr = device->CreateCommandQueue(
		&queueDesc, IID_PPV_ARGS(&queue_));
	assert(SUCCEEDED(hr));


	/// allocator---------------------------------
	for (uint32_t i = 0; i < kFrameCount_; ++i) {
		hr = device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator_[i]));
		assert(SUCCEEDED(hr));
	}


	/// list--------------------------------------
	hr = device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		allocator_[0].Get(), nullptr, IID_PPV_ARGS(&list_));
	assert(SUCCEEDED(hr));
	list_->Close();

	/// fence-------------------------------------
	for (uint32_t i = 0; i < kFrameCount_; ++i) {
		fenceValue_[i] = 0;
	}
	hr = device->CreateFence(fenceValue_[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	/// immediate allocator ------
	------------------
	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&immediateAllocator_));
	assert(SUCCEEDED(hr));


	/// immediate list -----------------------------
	hr = device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		immediateAllocator_.Get(),
		nullptr,
		IID_PPV_ARGS(&immediateList_));
	assert(SUCCEEDED(hr));


	/// immediate fence ----------------------------
	hr = device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&immediateFence_));
	assert(SUCCEEDED(hr));
	immediateFenceValue_ = 0;


	/// viewscissor-------------------------------

	viewport_.Width = MyWin::kWindowWidth;
	viewport_.Height = MyWin::kWindowHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	scissor_.left = 0;
	scissor_.right = MyWin::kWindowWidth;
	scissor_.top = 0;
	scissor_.bottom = MyWin::kWindowHeight;


}

void DXCommand::Flush() {
	globalFenceValue_++;
	fenceValue_[frameIndex_] = globalFenceValue_;
	const uint64_t fenceToWait = fenceValue_[frameIndex_];
	queue_->Signal(fence_.Get(), fenceToWait);

	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	fence_->SetEventOnCompletion(fenceToWait, fenceEvent);
	WaitForSingleObject(fenceEvent, INFINITE);
	CloseHandle(fenceEvent);
}

void DXCommand::Close(uint32_t index) {

	HRESULT hr;
	if (index == 0) {
		hr = list_->Close();
		assert(SUCCEEDED(hr));
	} else {
		hr = immediateList_->Close();
		assert(SUCCEEDED(hr));
	}
}

void DXCommand::Execution(uint32_t index) {

	if (index == 0) {
		ComPtr<ID3D12CommandList> commandLists[] = { list_.Get() };
		queue_->ExecuteCommandLists(1, commandLists->GetAddressOf());
	} else {
		ComPtr<ID3D12CommandList> commandLists[] = { immediateList_.Get() };
		queue_->ExecuteCommandLists(1, commandLists->GetAddressOf());
	}
}

void DXC::DXCommand::GPUSignal(uint32_t index) {
	// コマンドリストの実行完了を待つ
	if (index == 0) {
		globalFenceValue_++;
		fenceValue_[frameIndex_] = globalFenceValue_;
		const uint64_t fenceToWait = fenceValue_[frameIndex_];
		queue_->Signal(fence_.Get(), fenceToWait);
	} else {
		const uint64_t fenceToWait = ++immediateFenceValue_;
		queue_->Signal(immediateFence_.Get(), fenceToWait);
	}
}

void DXC::DXCommand::WaitForGPU(uint32_t frameIndex, uint32_t index) {
	frameIndex_ = frameIndex;

	if (index == 0) {
		if (fence_->GetCompletedValue() < fenceValue_[frameIndex_]) {

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			assert(fenceEvent != nullptr);

			fence_->SetEventOnCompletion(fenceValue_[frameIndex_], fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);
		}
	} else {
		if (immediateFence_->GetCompletedValue() < immediateFenceValue_) {

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			assert(fenceEvent != nullptr);

			immediateFence_->SetEventOnCompletion(immediateFenceValue_, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);
		}
	}

}

void DXCommand::Reset(uint32_t index) {

	HRESULT hr;

	if (index == 0) {
		hr = allocator_[frameIndex_]->Reset();
		assert(SUCCEEDED(hr));

		hr = list_->Reset(allocator_[frameIndex_].Get(), nullptr);
		assert(SUCCEEDED(hr));
	} else {
		hr = immediateAllocator_->Reset();
		assert(SUCCEEDED(hr));

		hr = immediateList_->Reset(immediateAllocator_.Get(), nullptr);
		assert(SUCCEEDED(hr));
	}
}

void DXCommand::SetViewAndscissor() {

	list_->RSSetViewports(1, &viewport_);
	list_->RSSetScissorRects(1, &scissor_);

}
