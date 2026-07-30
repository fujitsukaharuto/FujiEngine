#include "Engine/DXC/Command/CommandContext.h"
#include "Engine/Logger/Logger.h"

using namespace DXC;


CommandContext::~CommandContext() {
	Discard();
}

bool CommandContext::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {
	// assert は NDEBUG で消えるので、ログだけは全構成で出す
	auto fail = [&](const char* what, HRESULT hr) {
		Logger::Log(std::format("CommandContext::Initialize failed at {}. type={}, hr=0x{:08X}\n",
			what, static_cast<int>(type), static_cast<uint32_t>(hr)));
		Discard();
		return false;
		};

	// 1. キューの生成
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;
	HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue_));
	if (FAILED(hr)) { return fail("CreateCommandQueue", hr); }

	// 2. アロケータの生成（フレーム数分）
	for (uint32_t i = 0; i < DXC::kFrameCount_; ++i) {
		hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator_[i]));
		if (FAILED(hr)) { return fail("CreateCommandAllocator", hr); }
	}

	// 3. コマンドリストの生成
	hr = device->CreateCommandList(0, type, allocator_[0].Get(), nullptr, IID_PPV_ARGS(&list_));
	if (FAILED(hr)) { return fail("CreateCommandList", hr); }
	list_->Close(); // 最初は閉じた状態にしておく

	// 非対応環境でも起動は続けたいので、失敗しても nullptr のままにする
	if (FAILED(list_.As(&list4_))) {
		list4_ = nullptr;
	}

	// 4. フェンスとイベントの生成
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (FAILED(hr)) { return fail("CreateFence", hr); }
	for (uint32_t i = 0; i < DXC::kFrameCount_; ++i) {
		fenceValue_[i] = 0;
	}
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent_ == nullptr) { return fail("CreateEvent", HRESULT_FROM_WIN32(GetLastError())); }

	return true;
}

void CommandContext::Discard() {
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
	list4_.Reset();
	list_.Reset();
	for (uint32_t i = 0; i < kFrameCount_; ++i) {
		allocator_[i].Reset();
	}
	fence_.Reset();
	queue_.Reset();
}

void CommandContext::Close() {
	HRESULT hr = list_->Close();
	assert(SUCCEEDED(hr));
	isOpen_ = false;
}

void CommandContext::Execute() {
	ID3D12CommandList* commandLists[] = { list_.Get() };
	queue_->ExecuteCommandLists(1, commandLists);
}

void CommandContext::Signal(uint32_t frameIndex) {
	globalFenceValue_++;
	queue_->Signal(fence_.Get(), globalFenceValue_);
	fenceValue_[frameIndex] = globalFenceValue_;
}

void CommandContext::WaitForGPU(uint32_t frameIndex, bool isLogging) {
	if (fence_->GetCompletedValue() < fenceValue_[frameIndex]) {
		fence_->SetEventOnCompletion(fenceValue_[frameIndex], fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);

		if (isLogging) {
			Logger::LogF("frame=%llu idx=%u fence=%llu completed=%llu",
				globalFenceValue_,
				frameIndex,
				fenceValue_[frameIndex],
				fence_->GetCompletedValue());
		}
	}
}

void CommandContext::WaitQueueFor(CommandContext* otherContext, uint32_t frameIndex) {
	HRESULT hr = queue_->Wait(otherContext->GetFence(), otherContext->GetFenceValue(frameIndex));
	assert(SUCCEEDED(hr));
}

void CommandContext::Reset(uint32_t frameIndex) {
	HRESULT hr = allocator_[frameIndex]->Reset();
	assert(SUCCEEDED(hr));
	hr = list_->Reset(allocator_[frameIndex].Get(), nullptr);
	assert(SUCCEEDED(hr));
	isOpen_ = true;
}