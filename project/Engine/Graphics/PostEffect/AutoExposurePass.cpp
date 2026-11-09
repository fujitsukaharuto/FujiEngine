#include "Engine/Graphics/PostEffect/AutoExposurePass.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	// トーンマップが読む倍率
	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// 2x2 画素に1つ見る
	constexpr uint32_t kHistogramGroupPixels = 32;
}


void AutoExposurePass::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	// ヒストグラムは最初に UAV へ移したらそのまま
	histogram_ = Helper::CreateUAVResource(device, sizeof(uint32_t) * kBinCount_);
	histogramUavIndex_ = srvManager->Allocate();
	srvManager->CreateStructuredUAV(histogramUavIndex_, histogram_.Get(), kBinCount_, sizeof(uint32_t));

	exposure_ = Helper::CreateUAVResource(device, sizeof(float) * kResultCount_);
	exposureSrvIndex_ = srvManager->Allocate();
	exposureUavIndex_ = srvManager->Allocate();
	srvManager->CreateStructuredSRV(exposureSrvIndex_, exposure_.Get(), kResultCount_, sizeof(float));
	srvManager->CreateStructuredUAV(exposureUavIndex_, exposure_.Get(), kResultCount_, sizeof(float));

	paramResource_ = Helper::CreateBufferResource(device, kParamStride_ * kFrameCount_);
	paramResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_));

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		readback_[i] = Helper::CreateReadbackResource(device, sizeof(float) * kResultCount_);
	}

	ID3D12GraphicsCommandList* list = dxcommon_->GetImmediateList();
	D3D12_RESOURCE_BARRIER barriers[2]{};
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition.pResource = histogram_.Get();
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barriers[1] = barriers[0];
	barriers[1].Transition.pResource = exposure_.Get();
	barriers[1].Transition.StateAfter = kReadState;
	list->ResourceBarrier(2, barriers);
	dxcommon_->CommandExecution();

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_AutoExposure, "  Auto exposure");
}

void AutoExposurePass::Finalize() {
	gpuTimer_.Finalize();
	if (paramResource_) {
		paramMapped_ = nullptr;
		paramResource_.Reset();
	}
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		readback_[i].Reset();
	}
	histogram_.Reset();
	exposure_.Reset();
	dxcommon_ = nullptr;
}

void AutoExposurePass::SetSettings(const Settings& settings) {
	if (settings.enabled && !settings_.enabled) {
		needsReset_ = true;
	}
	settings_ = settings;
	settings_.maxEV = (std::max)(settings_.maxEV, settings_.minEV);
}

void AutoExposurePass::Render(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv) {
	if (dxcommon_ == nullptr || !settings_.enabled) {
		needsReset_ = true;
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	// この番号のフレームの GPU は待ち終わっている
	void* mapped = nullptr;
	const D3D12_RANGE readRange{ 0, sizeof(float) * kResultCount_ };
	if (SUCCEEDED(readback_[frameIndex]->Map(0, &readRange, &mapped))) {
		std::memcpy(result_, mapped, sizeof(result_));
		const D3D12_RANGE writeRange{ 0, 0 };
		readback_[frameIndex]->Unmap(0, &writeRange);
	}

	AutoExposureParams param;
	param.targetLuminance = kTargetLuminance_;
	param.minEV = settings_.minEV;
	param.maxEV = settings_.maxEV;
	param.lowPercent = kLowPercent_;
	param.highPercent = kHighPercent_;
	param.speedUp = settings_.speedUp;
	param.speedDown = settings_.speedDown;
	param.deltaTime = FPSKeeper::DeltaTime();
	param.resetHistory = needsReset_ ? 1u : 0u;
	std::memcpy(paramMapped_ + kParamStride_ * frameIndex, &param, sizeof(param));
	const D3D12_GPU_VIRTUAL_ADDRESS paramAddress = paramResource_->GetGPUVirtualAddress() + kParamStride_ * frameIndex;
	needsReset_ = false;

	gpuTimer_.Begin(cList, frameIndex, kTimer_AutoExposure);

	// 明るさを数える
	pPipeManager->SetCSPipeline(Pipe::AutoExposureHistogramCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kInputTexture, sceneSrv);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAutoExposureHistogram, srvManager->GetGPUDescriptorHandle(histogramUavIndex_));
	cList->Dispatch((MyWin::kWindowWidth + kHistogramGroupPixels - 1) / kHistogramGroupPixels,
		(MyWin::kWindowHeight + kHistogramGroupPixels - 1) / kHistogramGroupPixels, 1);

	D3D12_RESOURCE_BARRIER uavBarrier{};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = histogram_.Get();
	cList->ResourceBarrier(1, &uavBarrier);

	// 倍率を決めて寄せる
	dxcommon_->TransitionResource(exposure_.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::AutoExposureAverageCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kAutoExposureParams, paramAddress);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAutoExposureHistogram, srvManager->GetGPUDescriptorHandle(histogramUavIndex_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAutoExposureOut, srvManager->GetGPUDescriptorHandle(exposureUavIndex_));
	cList->Dispatch(1, 1, 1);
	cList->ResourceBarrier(1, &uavBarrier);

	// 表示用に CPU へ写す
	dxcommon_->TransitionResource(exposure_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cList->CopyBufferRegion(readback_[frameIndex].Get(), 0, exposure_.Get(), 0, sizeof(float) * kResultCount_);
	dxcommon_->TransitionResource(exposure_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, kReadState);

	gpuTimer_.End(cList, frameIndex, kTimer_AutoExposure);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_AutoExposure);
}

D3D12_GPU_DESCRIPTOR_HANDLE AutoExposurePass::GetExposureSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(exposureSrvIndex_);
}

void AutoExposurePass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Auto exposure")) {
		return;
	}
	if (settings_.enabled) {
		ImGui::Text("scene  : %.2f EV", result_[2]);
		ImGui::Text("target : %+.2f EV", result_[3]);
		ImGui::Text("current: %+.2f EV (x%.2f)", result_[1], result_[0]);
	} else {
		ImGui::TextDisabled("off");
	}
	ImGui::TreePop();
#endif // _DEBUGMODE
}
