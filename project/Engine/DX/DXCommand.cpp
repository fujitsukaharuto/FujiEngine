#include "DXCommand.h"
#include "MyWindow.h"
#include "Engine/Logger/Logger.h"
#include <cassert>

using namespace DXC;
using namespace Core;


DXCommand::~DXCommand() {
}


void DXCommand::Initialize(ID3D12Device* device) {
	graphicsContext_ = std::make_unique<CommandContext>();
	graphicsContext_->Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	// コンピュート用コンテキストの初期化
	computeContext_ = std::make_unique<CommandContext>();
	computeContext_->Initialize(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

	// 初期化用キュー
	immediateContext_ = std::make_unique<CommandContext>();
	immediateContext_->Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	immediateContext_->Reset(frameIndex_);

	/// viewScissor-------------------------------
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
	// Compute側のQueueのGPU実行を待つ
	computeContext_->Signal(frameIndex_);
	computeContext_->WaitForGPU(frameIndex_);

	// Graphics側のQueueのGPU実行を待つ
	graphicsContext_->Signal(frameIndex_);
	graphicsContext_->WaitForGPU(frameIndex_);
}

void DXCommand::Close(uint32_t index) {

	if (index == 0) {
		graphicsContext_->Close();
	} else {
		immediateContext_->Close();
	}
}

void DXCommand::Execution(uint32_t index) {

	if (index == 0) {
		graphicsContext_->Execute();
	} else {
		immediateContext_->Execute();
	}
}

void DXC::DXCommand::ComputeExecution() {
	computeContext_->Close();
	computeContext_->Execute();
}

void DXC::DXCommand::GPUSignal(uint32_t index) {
	// コマンドリストの実行完了を待つ
	if (index == 0) {
		graphicsContext_->Signal(frameIndex_);
	} else {
		immediateContext_->Signal(frameIndex_);
	}
}

void DXC::DXCommand::GPUComputeSignal() {
	computeContext_->Signal(frameIndex_);
}

void DXC::DXCommand::WaitForGPU(uint32_t index) {
	if (index == 0) {
		if (computeContext_->GetFenceValue(frameIndex_) != 0) { // ComputeQueueを待つ
			computeContext_->WaitForGPU(frameIndex_);
		}

		if (graphicsContext_->GetFenceValue(frameIndex_) != 0) { // DefaultQueueを待つ
			graphicsContext_->WaitForGPU(frameIndex_, true);
		}
	} else {
		immediateContext_->WaitForGPU(frameIndex_);
	}
}

void DXC::DXCommand::WaitComputeInGraphicsQueue() {
	graphicsContext_->WaitQueueFor(computeContext_.get(), frameIndex_);
}

void DXCommand::Reset(uint32_t index) {
	WaitForGPU();
	if (index == 0) {
		graphicsContext_->Reset(frameIndex_);

		computeContext_->Reset(frameIndex_);
	} else {
		immediateContext_->Reset(frameIndex_);
	}
}

void DXCommand::PerFrameWait() {
	uint32_t waitFrame = (frameIndex_ + kFrameCount_ - 1) % kFrameCount_;

	if (computeContext_->GetFenceValue(waitFrame) != 0) {
		computeContext_->WaitForGPU(waitFrame);
	}

	if (graphicsContext_->GetFenceValue(waitFrame) != 0) {
		graphicsContext_->WaitForGPU(waitFrame);
	}
}

void DXCommand::SetViewAndScissor(UINT width, UINT height) {
	D3D12_VIEWPORT viewport{ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT scissor{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	graphicsContext_->GetList()->RSSetViewports(1, &viewport);
	graphicsContext_->GetList()->RSSetScissorRects(1, &scissor);
}
