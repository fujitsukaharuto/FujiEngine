#include "DXCommand.h"
#include "MyWindow.h"
#include <cassert>

using namespace DXC;
using namespace Core;


DXCommand::~DXCommand() {

	list_.Reset();
	for (uint32_t i = 0; i < kFrameCount; ++i) {
		allocator_[i].Reset();
	}
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
	for (uint32_t i = 0; i < kFrameCount; ++i) {
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


	/// fence-------------------------------------
	fenceValue_ = 0;
	hr = device->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));


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


void DXCommand::Close() {

	HRESULT hr;

	hr = list_->Close();
	assert(SUCCEEDED(hr));

	ComPtr<ID3D12CommandList> commandLists[] = { list_.Get() };
	queue_->ExecuteCommandLists(1, commandLists->GetAddressOf());

}


void DXCommand::Execution() {

	// コマンドリストの実行完了を待つ
	fenceValue_++;
	queue_->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_) {

		HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(fenceEvent != nullptr);
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}

}


void DXCommand::Reset(uint32_t frameIndex) {

	HRESULT hr;

	frameIndex_ = frameIndex;

	hr = allocator_[frameIndex_]->Reset();
	assert(SUCCEEDED(hr));

	hr = list_->Reset(allocator_[frameIndex_].Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DXCommand::SetViewAndscissor() {

	list_->RSSetViewports(1, &viewport_);
	list_->RSSetScissorRects(1, &scissor_);

}
