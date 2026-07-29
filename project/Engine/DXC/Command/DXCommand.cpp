#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Logger/Logger.h"
#include <cassert>

using namespace DXC;
using namespace Core;


DXCommand::~DXCommand() {
}


void DXCommand::Initialize(ID3D12Device* device) {
	// DIRECT も COMPUTE も D3D12 が全デバイスで必須としている種別なので、
	// 失敗＝デバイスが立ち行かない状況。復帰はさせずログだけ必ず残す
	// （assert は Develop/Release では消えるため、これが唯一の手掛かりになる）
	auto initContext = [&](std::unique_ptr<CommandContext>& context,
		D3D12_COMMAND_LIST_TYPE type, const char* name) {
			context = std::make_unique<CommandContext>();
			if (!context->Initialize(device, type)) {
				Logger::Log(std::format("FATAL: mandatory command queue '{}' could not be created.\n", name));
				assert(false);
			}
		};

	initContext(graphicsContext_, D3D12_COMMAND_LIST_TYPE_DIRECT, "graphics");

	// コンピュート用コンテキストの初期化
	initContext(computeContext_, D3D12_COMMAND_LIST_TYPE_COMPUTE, "compute");

	// 初期化用キュー
	initContext(immediateContext_, D3D12_COMMAND_LIST_TYPE_DIRECT, "immediate");
	immediateContext_->Reset(frameIndex_);

	// 加速構造の構築先を決める唯一の場所。
	// 既定は描画と同じQueue＝フェンス不要で最も単純に動く形。専用Queueを起こすときは
	// ここでそれを生成して代入する（同期側は IsASBuildOnGraphicsQueue() が拾う）
	asBuildContext_ = graphicsContext_.get();

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

void DXCommand::Close(SubmitPath path) {

	if (path == SubmitPath::Frame) {
		graphicsContext_->Close();
	} else {
		immediateContext_->Close();
	}
}

void DXCommand::Execution(SubmitPath path) {

	if (path == SubmitPath::Frame) {
		graphicsContext_->Execute();
	} else {
		immediateContext_->Execute();
	}
}

void DXC::DXCommand::ComputeExecution() {
	computeContext_->Close();
	computeContext_->Execute();
}

void DXC::DXCommand::GPUSignal(SubmitPath path) {
	// コマンドリストの実行完了を待つ
	if (path == SubmitPath::Frame) {
		graphicsContext_->Signal(frameIndex_);
	} else {
		immediateContext_->Signal(frameIndex_);
	}
}

void DXC::DXCommand::GPUComputeSignal() {
	computeContext_->Signal(frameIndex_);
}

void DXC::DXCommand::WaitForGPU(SubmitPath path) {
	if (path == SubmitPath::Frame) {
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

void DXC::DXCommand::WaitGraphicsInComputeQueue() {
	// 前フレーム(=同じプールを読み終えたフレーム)のGraphics完了を待つ。
	uint32_t prevFrame = (frameIndex_ + kFrameCount_ - 1) % kFrameCount_;
	if (graphicsContext_->GetFenceValue(prevFrame) != 0) {
		computeContext_->WaitQueueFor(graphicsContext_.get(), prevFrame);
	}
}

void DXCommand::Reset(SubmitPath path) {
	WaitForGPU();
	if (path == SubmitPath::Frame) {
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
