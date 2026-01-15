#include "DXCommand.h"
#include "MyWindow.h"
#include "Engine/Logger/Logger.h"
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


	/// ------------------------------------------
	/// Compute Queue の追加
	/// ------------------------------------------
	D3D12_COMMAND_QUEUE_DESC computeQueueDesc{};
	computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	hr = device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&computeQueue_));
	assert(SUCCEEDED(hr));

	for (uint32_t i = 0; i < kFrameCount_; ++i) {
		hr = device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			IID_PPV_ARGS(&computeAllocator_[i]));
		assert(SUCCEEDED(hr));
	}

	hr = device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
		computeAllocator_[0].Get(), nullptr, IID_PPV_ARGS(&computeList_));
	assert(SUCCEEDED(hr));
	computeList_->Close();

	for (uint32_t i = 0; i < kFrameCount_; ++i) {
		computeFenceValue_[i] = 0;
	}
	hr = device->CreateFence(computeFenceValue_[0], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&computeFence_));
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
	{
		globalComputeFenceValue_++;
		computeFenceValue_[frameIndex_] = globalComputeFenceValue_;
		const uint64_t fenceToWait = computeFenceValue_[frameIndex_];
		computeQueue_->Signal(computeFence_.Get(), fenceToWait);

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		computeFence_->SetEventOnCompletion(fenceToWait, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}
	{
		globalFenceValue_++;
		fenceValue_[frameIndex_] = globalFenceValue_;
		const uint64_t fenceToWait = fenceValue_[frameIndex_];
		queue_->Signal(fence_.Get(), fenceToWait);

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		fence_->SetEventOnCompletion(fenceToWait, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}
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

void DXC::DXCommand::ComputeExecution() {
	HRESULT hr;
	hr = computeList_->Close();
	assert(SUCCEEDED(hr));
	ComPtr<ID3D12CommandList> computeCommandLists[] = { computeList_.Get() };
	computeQueue_->ExecuteCommandLists(1, computeCommandLists->GetAddressOf());
}

void DXC::DXCommand::GPUSignal(uint32_t index) {
	// コマンドリストの実行完了を待つ
	if (index == 0) {
		globalFenceValue_++;
		queue_->Signal(fence_.Get(), globalFenceValue_);
		fenceValue_[frameIndex_] = globalFenceValue_;
	} else {
		const uint64_t fenceToWait = ++immediateFenceValue_;
		queue_->Signal(immediateFence_.Get(), fenceToWait);
	}
}

void DXC::DXCommand::GPUComputeSignal() {
	globalComputeFenceValue_++;
	computeQueue_->Signal(computeFence_.Get(), globalComputeFenceValue_);
	computeFenceValue_[frameIndex_] = globalComputeFenceValue_;
}

void DXC::DXCommand::WaitForGPU(uint32_t index) {

	if (index == 0) {
		if (computeFenceValue_[frameIndex_] != 0) {
			if (computeFence_->GetCompletedValue() < computeFenceValue_[frameIndex_]) {
				HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				assert(fenceEvent != nullptr);

				computeFence_->SetEventOnCompletion(computeFenceValue_[frameIndex_], fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
				CloseHandle(fenceEvent);
			}
		}

		if (fenceValue_[frameIndex_] != 0) {
			if (fence_->GetCompletedValue() < fenceValue_[frameIndex_]) {

				HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				assert(fenceEvent != nullptr);

				fence_->SetEventOnCompletion(fenceValue_[frameIndex_], fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
				CloseHandle(fenceEvent);
				Logger::LogF("frame=%llu idx=%u fence=%llu completed=%llu",
					globalFenceValue_,
					frameIndex_,
					fenceValue_[frameIndex_],
					fence_->GetCompletedValue());
			}
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

void DXC::DXCommand::WaitComputeInGraphicsQueue() {
	queue_->Wait(computeFence_.Get(), computeFenceValue_[frameIndex_]);
}

void DXCommand::Reset(uint32_t index) {

	HRESULT hr;

	WaitForGPU();
	if (index == 0) {
		hr = allocator_[frameIndex_]->Reset();
		assert(SUCCEEDED(hr));
		hr = list_->Reset(allocator_[frameIndex_].Get(), nullptr);
		assert(SUCCEEDED(hr));

		hr = computeAllocator_[frameIndex_]->Reset();
		assert(SUCCEEDED(hr));
		hr = computeList_->Reset(computeAllocator_[frameIndex_].Get(), nullptr);
		assert(SUCCEEDED(hr));
	} else {
		hr = immediateAllocator_->Reset();
		assert(SUCCEEDED(hr));

		hr = immediateList_->Reset(immediateAllocator_.Get(), nullptr);
		assert(SUCCEEDED(hr));
	}
}

void DXCommand::PerFrameWait() {
	uint32_t waitFrame = (frameIndex_ + kFrameCount_ - 1) % kFrameCount_;

	if (computeFenceValue_[waitFrame] != 0) {
		if (computeFence_->GetCompletedValue() < computeFenceValue_[waitFrame]) {
			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			assert(fenceEvent != nullptr);

			computeFence_->SetEventOnCompletion(computeFenceValue_[waitFrame], fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);
		}
	}

	if (fenceValue_[waitFrame] != 0) {
		if (fence_->GetCompletedValue() < fenceValue_[waitFrame]) {

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			assert(fenceEvent != nullptr);

			fence_->SetEventOnCompletion(fenceValue_[waitFrame], fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);
		}
	}
}

void DXCommand::SetViewAndscissor() {

	list_->RSSetViewports(1, &viewport_);
	list_->RSSetScissorRects(1, &scissor_);

}
