#include "Engine/Graphics/PostEffect/BloomPass.h"

#include <algorithm>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
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

	// 書かないときの置き場所。次の段がCSから読む
	constexpr D3D12_RESOURCE_STATES kReadState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	constexpr uint32_t kThreadGroupSize = 8;

	uint32_t DispatchCount(uint32_t pixels) {
		return (pixels + kThreadGroupSize - 1) / kThreadGroupSize;
	}
}


void BloomPass::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	ID3D12Device* device = dxcommon_->GetDevice();

	uint32_t width = static_cast<uint32_t>(MyWin::kWindowWidth);
	uint32_t height = static_cast<uint32_t>(MyWin::kWindowHeight);
	for (uint32_t i = 0; i < kMipCount_; i++) {
		width = (std::max)(width / 2, 1u);
		height = (std::max)(height / 2, 1u);
		CreateMip(down_[i], width, height);
		if (i < kMipCount_ - 1) {
			CreateMip(up_[i], width, height);
		}
	}

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, kParamStride_ * kParamSlotCount_);
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
}

void BloomPass::Finalize() {
	gpuTimer_.Finalize();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	for (Mip& mip : down_) { mip.resource.Reset(); }
	for (Mip& mip : up_) { mip.resource.Reset(); }
	dxcommon_ = nullptr;
}

void BloomPass::CreateMip(Mip& mip, uint32_t width, uint32_t height) {
	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	mip.width = width;
	mip.height = height;
	mip.resource = Helper::CreateTexture2D(device, static_cast<int32_t>(width), static_cast<int32_t>(height),
		kSceneColorFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState, nullptr);

	mip.srvIndex = srvManager->Allocate();
	mip.uavIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(mip.srvIndex, mip.resource.Get(), kSceneColorFormat, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = kSceneColorFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(mip.resource.Get(), nullptr, &uavDesc,
		srvManager->GetCPUDescriptorHandle(mip.uavIndex));
}

D3D12_GPU_VIRTUAL_ADDRESS BloomPass::WriteParam(uint32_t frameIndex, uint32_t slot, const BloomPassParam& param) {
	const uint64_t offset = kParamStride_ * slot;
	*reinterpret_cast<BloomPassParam*>(paramMapped_[frameIndex] + offset) = param;
	return paramResource_[frameIndex]->GetGPUVirtualAddress() + offset;
}

void BloomPass::Render(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE outputUav,
	float threshold, float intensity) {
	if (dxcommon_ == nullptr) {
		return;
	}

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	BloomPassParam param{};
	param.threshold = threshold;
	// 0 だとシェーダの smoothstep の両端が一致してしまう
	param.knee = (std::max)(knee_, 0.001f);
	param.scatter = scatter_;
	param.intensity = intensity;

	uint32_t slot = 0;
	gpuTimer_.Begin(cList, frameIndex, kTimer_Bloom);

	// --- 縮小。最初の1回は元のシーンから明るい所を抜き出しながら縮める
	D3D12_GPU_DESCRIPTOR_HANDLE source = sceneSrv;
	uint32_t sourceWidth = static_cast<uint32_t>(MyWin::kWindowWidth);
	uint32_t sourceHeight = static_cast<uint32_t>(MyWin::kWindowHeight);

	pPipeManager->SetCSPipeline(Pipe::BloomDownsampleCS);
	for (uint32_t i = 0; i < kMipCount_; i++) {
		const Mip& dst = down_[i];

		param.srcTexelSize[0] = 1.0f / static_cast<float>(sourceWidth);
		param.srcTexelSize[1] = 1.0f / static_cast<float>(sourceHeight);
		param.dstSize[0] = dst.width;
		param.dstSize[1] = dst.height;
		param.prefilter = (i == 0) ? 1u : 0u;

		dxcommon_->TransitionResource(dst.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pPipeManager->SetComputeRootCBV(cList, RootName::kBloomPassParam, WriteParam(frameIndex, slot++, param));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kBloomSource, source);
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kBloomOutput, srvManager->GetGPUDescriptorHandle(dst.uavIndex));
		cList->Dispatch(DispatchCount(dst.width), DispatchCount(dst.height), 1);

		dxcommon_->TransitionResource(dst.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

		source = srvManager->GetGPUDescriptorHandle(dst.srvIndex);
		sourceWidth = dst.width;
		sourceHeight = dst.height;
	}
	param.prefilter = 0;

	// --- 拡大。いちばん小さい段から始めて、1段ずつ大きい方と混ぜながら戻す
	const Mip* low = &down_[kMipCount_ - 1];

	pPipeManager->SetCSPipeline(Pipe::BloomUpsampleCS);
	for (int32_t i = static_cast<int32_t>(kMipCount_) - 2; i >= 0; i--) {
		const Mip& dst = up_[i];

		param.srcTexelSize[0] = 1.0f / static_cast<float>(low->width);
		param.srcTexelSize[1] = 1.0f / static_cast<float>(low->height);
		param.dstSize[0] = dst.width;
		param.dstSize[1] = dst.height;

		dxcommon_->TransitionResource(dst.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pPipeManager->SetComputeRootCBV(cList, RootName::kBloomPassParam, WriteParam(frameIndex, slot++, param));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kBloomLow, srvManager->GetGPUDescriptorHandle(low->srvIndex));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kBloomHigh, srvManager->GetGPUDescriptorHandle(down_[i].srvIndex));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kBloomOutput, srvManager->GetGPUDescriptorHandle(dst.uavIndex));
		cList->Dispatch(DispatchCount(dst.width), DispatchCount(dst.height), 1);

		dxcommon_->TransitionResource(dst.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

		low = &dst;
	}

	// --- 合成。積み上げたぼけを画面の大きさへ拡大して元のシーンへ足す
	param.srcTexelSize[0] = 1.0f / static_cast<float>(low->width);
	param.srcTexelSize[1] = 1.0f / static_cast<float>(low->height);
	param.dstSize[0] = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.dstSize[1] = static_cast<uint32_t>(MyWin::kWindowHeight);

	pPipeManager->SetCSPipeline(Pipe::BloomCompositeCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kBloomPassParam, WriteParam(frameIndex, slot++, param));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kInputTexture, sceneSrv);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kBloomLow, srvManager->GetGPUDescriptorHandle(low->srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kOutputTexture, outputUav);
	cList->Dispatch(DispatchCount(param.dstSize[0]), DispatchCount(param.dstSize[1]), 1);

	gpuTimer_.End(cList, frameIndex, kTimer_Bloom);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Bloom);
}

void BloomPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (dxcommon_ != nullptr) {
		// GPUが書き終わっているのは1つ前のフレームぶん
		const uint32_t finishedFrame = (dxcommon_->GetNowFrameCount() + kFrameCount_ - 1) % kFrameCount_;
		ImGui::Text("bloom : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Bloom));
	}
#endif // _DEBUGMODE
}
