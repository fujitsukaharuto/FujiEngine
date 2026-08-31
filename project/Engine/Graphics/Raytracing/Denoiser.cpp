#include "Engine/Graphics/Raytracing/Denoiser.h"

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	// 遮蔽率は0〜1の1チャンネル。R8だと平坦な階調に段差が見えるのでFP16
	constexpr DXGI_FORMAT kSignalFormat = DXGI_FORMAT_R16_FLOAT;
	// 蓄積は x=信号, y=蓄積フレーム数
	constexpr DXGI_FORMAT kHistoryFormat = DXGI_FORMAT_R16G16_FLOAT;

	constexpr uint32_t kThreadGroupSize = 8;

	uint32_t DispatchCount(int32_t pixels) {
		return (static_cast<uint32_t>(pixels) + kThreadGroupSize - 1) / kThreadGroupSize;
	}
}


void Denoiser::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	ID3D12Device* device = dxcommon_->GetDevice();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		history_[i].Create(device, kHistoryFormat, false);
	}
	// 読み手は結果のテクスチャを無条件に引くので、一度も書かれないフレーム
	// (RayQuery非対応機や、この信号を切っている間)でも中身が定義されている必要がある
	for (uint32_t i = 0; i < 2; i++) {
		filter_[i].Create(device, kSignalFormat, true);
	}
	finalSrvIndex_ = filter_[0].srvIndex;

	clearRtvHeap_ = Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = kSignalFormat;
	clearValue.Color[0] = 1.0f;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = kSignalFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// 初期化時はまだ描画のコマンドリストが開いていないので immediate 経路で流す。
	// DXCom::TransitionResource は描画リストに積むので、ここではバリアも自前で immediate へ積む
	ID3D12GraphicsCommandList* immediateList = dxcommon_->GetImmediateList();
	const UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = clearRtvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_RESOURCE_BARRIER barriers[2]{};
	for (uint32_t i = 0; i < 2; i++) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtv{ rtvStart.ptr + i * rtvSize };
		device->CreateRenderTargetView(filter_[i].resource.Get(), &rtvDesc, rtv);
		immediateList->ClearRenderTargetView(rtv, clearValue.Color, 0, nullptr);

		barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[i].Transition.pResource = filter_[i].resource.Get();
		barriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barriers[i].Transition.StateAfter = WorkTexture::kReadState;
		barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}
	immediateList->ResourceBarrier(2, barriers);

	dxcommon_->CommandExecution();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		temporalResource_[i] = Helper::CreateBufferResource(device, sizeof(DenoiseTemporalParam));
		temporalResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&temporalMapped_[i]));

		spatialResource_[i] = Helper::CreateBufferResource(device,
			kSpatialParamStride_ * kMaxSpatialIterations_ * kSpatialPassesPerIteration_);
		spatialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&spatialMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
}

void Denoiser::Finalize() {
	gpuTimer_.Finalize();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		temporalMapped_[i] = nullptr;
		temporalResource_[i].Reset();
		spatialMapped_[i] = nullptr;
		spatialResource_[i].Reset();
		history_[i].Reset();
	}
	for (uint32_t i = 0; i < 2; i++) {
		filter_[i].Reset();
	}
	clearRtvHeap_.Reset();
	dxcommon_ = nullptr;
}

void Denoiser::Render(const GBufferPass& gbuffer, const WorkTexture& raw) {
	if (dxcommon_ == nullptr) {
		hasHistory_ = false;
		return;
	}

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateParam(frameIndex, gbuffer);

	// 両方を切ると生の信号がそのまま出る。粒の量を見比べるとき用
	uint32_t srcSrvIndex = raw.srvIndex;
	finalSrvIndex_ = raw.srvIndex;
	uint32_t nextDst = 0;

	if (enableTemporal_) {
		DispatchTemporal(frameIndex, gbuffer, raw);
		srcSrvIndex = filter_[0].srvIndex;
		finalSrvIndex_ = filter_[0].srvIndex;
		nextDst = 1;
	}
	hasHistory_ = enableTemporal_;

	uint32_t iterations = static_cast<uint32_t>(spatialIterations_);
	if (iterations > kMaxSpatialIterations_) {
		iterations = kMaxSpatialIterations_;
	}

	if (iterations > 0) {
		ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
		gpuTimer_.Begin(cList, frameIndex, kTimer_Spatial);

		uint32_t paramSlot = 0;
		for (uint32_t i = 0; i < iterations; i++) {
			// 5x5 のカーネルは外積なので横と縦に割れる。1画素あたり 25タップが 5+5 タップになる
			for (uint32_t axis = 0; axis < kSpatialPassesPerIteration_; axis++) {
				const WorkTexture& dst = filter_[nextDst];
				DispatchSpatial(frameIndex, gbuffer, i, paramSlot, axis != 0, srcSrvIndex, dst);
				srcSrvIndex = dst.srvIndex;
				finalSrvIndex_ = dst.srvIndex;
				nextDst = 1 - nextDst;
				paramSlot++;
			}
		}

		gpuTimer_.End(cList, frameIndex, kTimer_Spatial);
		gpuTimer_.Resolve(cList, frameIndex, kTimer_Spatial);
	}
}

void Denoiser::UpdateParam(uint32_t frameIndex, const GBufferPass& gbuffer) {
	DenoiseTemporalParam temporal{};
	temporal.prevViewProj = prevViewProj_;
	temporal.prevInvViewProj = prevInvViewProj_;
	temporal.screenWidth = static_cast<uint32_t>(MyWin::kWindowWidth);
	temporal.screenHeight = static_cast<uint32_t>(MyWin::kWindowHeight);
	// 前フレームのG-Bufferが無いと履歴の妥当性を確かめられない。パスを入れ直した直後がこれに当たる
	temporal.hasHistory = (hasHistory_ && gbuffer.IsPreviousRendered()) ? 1u : 0u;
	temporal.maxHistoryLength = maxHistoryLength_;
	temporal.normalThreshold = normalThreshold_;
	temporal.positionThreshold = positionThreshold_;

	*temporalMapped_[frameIndex] = temporal;

	// 次フレームの再投影に使う
	prevViewProj_ = CameraManager::GetInstance()->GetCamera()->GetViewProjectionMatrix();
	prevInvViewProj_ = Math::Inverse(prevViewProj_);
}

void Denoiser::DispatchTemporal(uint32_t frameIndex, const GBufferPass& gbuffer, const WorkTexture& raw) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	const WorkTexture& historyOut = history_[frameIndex];
	const WorkTexture& historyPrev = history_[(frameIndex + kFrameCount_ - 1) % kFrameCount_];

	dxcommon_->TransitionResource(historyOut.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dxcommon_->TransitionResource(filter_[0].resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	gpuTimer_.Begin(cList, frameIndex, kTimer_Temporal);

	pPipeManager->SetCSPipeline(Pipe::DenoiseTemporalCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo,
		CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kDenoiseTemporalParam,
		temporalResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormalPrev, gbuffer.GetNormalSrvHandle(true));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepthPrev, gbuffer.GetDepthSrvHandle(true));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDenoiseRaw, srvManager->GetGPUDescriptorHandle(raw.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDenoiseHistoryPrev, srvManager->GetGPUDescriptorHandle(historyPrev.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDenoiseHistoryOut, srvManager->GetGPUDescriptorHandle(historyOut.uavIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDenoiseFilterOut, srvManager->GetGPUDescriptorHandle(filter_[0].uavIndex));

	cList->Dispatch(DispatchCount(MyWin::kWindowWidth), DispatchCount(MyWin::kWindowHeight), 1);

	gpuTimer_.End(cList, frameIndex, kTimer_Temporal);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Temporal);
	dxcommon_->TransitionResource(historyOut.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
	dxcommon_->TransitionResource(filter_[0].resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void Denoiser::DispatchSpatial(uint32_t frameIndex, const GBufferPass& gbuffer,
	uint32_t iteration, uint32_t paramSlot, bool vertical,
	uint32_t srcSrvIndex, const WorkTexture& dst) {

	DenoiseSpatialParam param{};
	param.screenWidth = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenHeight = static_cast<uint32_t>(MyWin::kWindowHeight);
	param.stepWidth = 1u << iteration;
	param.normalPower = normalPower_;
	param.planeScale = planeScale_;
	param.tapDirection[0] = vertical ? 0 : 1;
	param.tapDirection[1] = vertical ? 1 : 0;

	const uint64_t offset = kSpatialParamStride_ * paramSlot;
	*reinterpret_cast<DenoiseSpatialParam*>(spatialMapped_[frameIndex] + offset) = param;

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	// 入力は前段が読み取り状態で置いていったもの。書き込み先だけを入れ替える
	dxcommon_->TransitionResource(dst.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pPipeManager->SetCSPipeline(Pipe::DenoiseSpatialCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo,
		CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kDenoiseSpatialParam,
		spatialResource_[frameIndex]->GetGPUVirtualAddress() + offset);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDenoiseFilterInput, srvManager->GetGPUDescriptorHandle(srcSrvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDenoiseFilterOut, srvManager->GetGPUDescriptorHandle(dst.uavIndex));

	cList->Dispatch(DispatchCount(MyWin::kWindowWidth), DispatchCount(MyWin::kWindowHeight), 1);

	dxcommon_->TransitionResource(dst.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void Denoiser::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Denoiser")) {
		return;
	}

	ImGui::Checkbox("Temporal (reprojection)", &enableTemporal_);
	ImGui::SliderInt("Spatial iterations", &spatialIterations_, 0, static_cast<int>(kMaxSpatialIterations_));

	ImGui::SeparatorText("Temporal");
	ImGui::SliderFloat("max history", &maxHistoryLength_, 1.0f, 64.0f, "%.0f frames");
	ImGui::SliderFloat("normal threshold", &normalThreshold_, 0.0f, 1.0f);
	ImGui::SliderFloat("position threshold", &positionThreshold_, 0.0f, 0.1f, "%.4f");

	ImGui::SeparatorText("Spatial");
	ImGui::SliderFloat("normal power", &normalPower_, 1.0f, 128.0f);
	ImGui::SliderFloat("plane scale", &planeScale_, 0.0f, 0.1f, "%.4f");

	ImGui::TreePop();
#endif // _DEBUGMODE
}

D3D12_GPU_DESCRIPTOR_HANDLE Denoiser::GetResultSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(finalSrvIndex_);
}

double Denoiser::GetTemporalMS() {
	if (dxcommon_ == nullptr) { return 0.0; }
	// GPUが書き終わっているのは1つ前のフレームぶん
	const uint32_t finishedFrame = (dxcommon_->GetNowFrameCount() + kFrameCount_ - 1) % kFrameCount_;
	return gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Temporal);
}

double Denoiser::GetSpatialMS() {
	if (dxcommon_ == nullptr) { return 0.0; }
	const uint32_t finishedFrame = (dxcommon_->GetNowFrameCount() + kFrameCount_ - 1) % kFrameCount_;
	return gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Spatial);
}
