#include "Engine/Graphics/Raytracing/RayTracedAOPass.h"

#include <cmath>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	// AOは0〜1の1チャンネル。R8だと平坦な階調に段差が見えるのでFP16
	constexpr DXGI_FORMAT kAOFormat = DXGI_FORMAT_R16_FLOAT;
	// 蓄積は x=AO, y=蓄積フレーム数
	constexpr DXGI_FORMAT kHistoryFormat = DXGI_FORMAT_R16G16_FLOAT;

	// 前方描画(PS)とデノイザ(CS)の両方から読むので読み取り状態を合成しておく
	constexpr D3D12_RESOURCE_STATES kAOReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// 黄金比の共役。フレームごとにこれを足して剰余を取ると、少ない回数でも一様に散る
	constexpr double kGoldenRatioConjugate = 0.6180339887498949;

	constexpr uint32_t kThreadGroupSize = 8;

	uint32_t DispatchCount(int32_t pixels) {
		return (static_cast<uint32_t>(pixels) + kThreadGroupSize - 1) / kThreadGroupSize;
	}
}


void RayTracedAOPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;

	ID3D12Device* device = dxcommon_->GetDevice();

	raw_ = CreateWorkTexture(kAOFormat, false);
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		history_[i] = CreateWorkTexture(kHistoryFormat, false);
	}
	// 前方描画は gAOTexture を無条件に読むので、一度も書かれないフレーム
	// (RayQuery非対応機やAOを切っている間)でも中身が定義されている必要がある
	for (uint32_t i = 0; i < 2; i++) {
		filter_[i] = CreateWorkTexture(kAOFormat, true);
	}
	finalSrvIndex_ = filter_[0].srvIndex;

	clearRtvHeap_ = Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = kAOFormat;
	clearValue.Color[0] = 1.0f;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = kAOFormat;
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
		barriers[i].Transition.StateAfter = kAOReadState;
		barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}
	immediateList->ResourceBarrier(2, barriers);

	dxcommon_->CommandExecution();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(AOPassParam));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));

		temporalResource_[i] = Helper::CreateBufferResource(device, sizeof(DenoiseTemporalParam));
		temporalResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&temporalMapped_[i]));

		spatialResource_[i] = Helper::CreateBufferResource(device,
			kSpatialParamStride_ * kMaxSpatialIterations_ * kSpatialPassesPerIteration_);
		spatialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&spatialMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
}

RayTracedAOPass::WorkTexture RayTracedAOPass::CreateWorkTexture(DXGI_FORMAT format, bool clearToOpen) {
	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	D3D12_RESOURCE_STATES state = kAOReadState;
	D3D12_CLEAR_VALUE clearValue{};
	const D3D12_CLEAR_VALUE* clearValuePtr = nullptr;

	// クリアは呼び出し側がまとめて掛ける。ここではRTVを許可した状態で生成するだけ
	if (clearToOpen) {
		flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		state = D3D12_RESOURCE_STATE_RENDER_TARGET;
		clearValue.Format = format;
		clearValue.Color[0] = 1.0f;
		clearValuePtr = &clearValue;
	}

	WorkTexture texture{};
	texture.resource = Helper::CreateTexture2D(device, MyWin::kWindowWidth, MyWin::kWindowHeight,
		format, flags, state, clearValuePtr);

	texture.srvIndex = srvManager->Allocate();
	texture.uavIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(texture.srvIndex, texture.resource.Get(), format, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(texture.resource.Get(), nullptr, &uavDesc,
		srvManager->GetCPUDescriptorHandle(texture.uavIndex));

	return texture;
}

void RayTracedAOPass::Finalize() {
	gpuTimer_.Finalize();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
		temporalMapped_[i] = nullptr;
		temporalResource_[i].Reset();
		spatialMapped_[i] = nullptr;
		spatialResource_[i].Reset();
		history_[i].resource.Reset();
	}
	for (uint32_t i = 0; i < 2; i++) {
		filter_[i].resource.Reset();
	}
	clearRtvHeap_.Reset();
	raw_.resource.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void RayTracedAOPass::Render(const GBufferPass& gbuffer) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !gbuffer.IsRendered()) {
		hasHistory_ = false;
		return;
	}

	auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene();
	if (rtScene == nullptr || !rtScene->IsAvailable()) {
		hasHistory_ = false;
		return;
	}
	const uint32_t tlasSrv = rtScene->GetTlasSrvIndex();
	if (tlasSrv == RaytracingScene::kInvalidSrvIndex) {
		hasHistory_ = false;
		return;
	}

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateParam(frameIndex, gbuffer);

	DispatchTrace(frameIndex, gbuffer, tlasSrv);

	// デノイズを全て切ると生AOがそのまま出る。粒の量を見比べるとき用
	uint32_t srcSrvIndex = raw_.srvIndex;
	finalSrvIndex_ = raw_.srvIndex;
	uint32_t nextDst = 0;

	if (enableTemporal_) {
		DispatchTemporal(frameIndex, gbuffer);
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

void RayTracedAOPass::DispatchTrace(uint32_t frameIndex, const GBufferPass& gbuffer, uint32_t tlasSrvIndex) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(raw_.resource.Get(), kAOReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);

	pPipeManager->SetCSPipeline(Pipe::RayTracedAOCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo,
		CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kAOPassParam, paramResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(tlasSrvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAOOutput, srvManager->GetGPUDescriptorHandle(raw_.uavIndex));

	cList->Dispatch(DispatchCount(MyWin::kWindowWidth), DispatchCount(MyWin::kWindowHeight), 1);

	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);
	dxcommon_->TransitionResource(raw_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kAOReadState);
}

void RayTracedAOPass::DispatchTemporal(uint32_t frameIndex, const GBufferPass& gbuffer) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	const WorkTexture& historyOut = history_[frameIndex];
	const WorkTexture& historyPrev = history_[(frameIndex + kFrameCount_ - 1) % kFrameCount_];

	dxcommon_->TransitionResource(historyOut.resource.Get(), kAOReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dxcommon_->TransitionResource(filter_[0].resource.Get(), kAOReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	gpuTimer_.Begin(cList, frameIndex, kTimer_Temporal);

	pPipeManager->SetCSPipeline(Pipe::AODenoiseTemporalCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo,
		CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kDenoiseTemporalParam,
		temporalResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormalPrev, gbuffer.GetNormalSrvHandle(true));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepthPrev, gbuffer.GetDepthSrvHandle(true));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAORaw, srvManager->GetGPUDescriptorHandle(raw_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAOHistoryPrev, srvManager->GetGPUDescriptorHandle(historyPrev.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAOHistoryOut, srvManager->GetGPUDescriptorHandle(historyOut.uavIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAOFilterOut, srvManager->GetGPUDescriptorHandle(filter_[0].uavIndex));

	cList->Dispatch(DispatchCount(MyWin::kWindowWidth), DispatchCount(MyWin::kWindowHeight), 1);

	gpuTimer_.End(cList, frameIndex, kTimer_Temporal);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Temporal);
	dxcommon_->TransitionResource(historyOut.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kAOReadState);
	dxcommon_->TransitionResource(filter_[0].resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kAOReadState);
}

void RayTracedAOPass::DispatchSpatial(uint32_t frameIndex, const GBufferPass& gbuffer,
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
	dxcommon_->TransitionResource(dst.resource.Get(), kAOReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pPipeManager->SetCSPipeline(Pipe::AODenoiseSpatialCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo,
		CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kDenoiseSpatialParam,
		spatialResource_[frameIndex]->GetGPUVirtualAddress() + offset);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAOFilterInput, srvManager->GetGPUDescriptorHandle(srcSrvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAOFilterOut, srvManager->GetGPUDescriptorHandle(dst.uavIndex));

	cList->Dispatch(DispatchCount(MyWin::kWindowWidth), DispatchCount(MyWin::kWindowHeight), 1);

	dxcommon_->TransitionResource(dst.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kAOReadState);
}

void RayTracedAOPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("RayTraced AO (screen space)")) {
		return;
	}

	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	// GPUが書き終わっているのは1つ前のフレームぶん
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace    : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("temporal : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Temporal));
	ImGui::Text("spatial  : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Spatial));

	if (ImGui::TreeNode("Denoiser")) {
		ImGui::TextWrapped("両方を切ると生AOがそのまま出る。粒の量を見比べるとき用");
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
	}

	static float scale = 0.25f;
	ImGui::SliderFloat("preview scale##ao", &scale, 0.1f, 1.0f);
	const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);

	ImGui::Text("Raw");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(raw_.srvIndex).ptr), size);
	ImGui::Text("Denoised (this is what the forward pass reads)");
	ImGui::Image(static_cast<ImTextureID>(GetAOSrvHandle().ptr), size);
	ImGui::TextWrapped("黒いほど遮蔽されている。Lights > RayTraced AO の Mode が Screen のときだけ更新される");

	ImGui::TreePop();
#endif // _DEBUGMODE
}

D3D12_GPU_DESCRIPTOR_HANDLE RayTracedAOPass::GetAOSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(finalSrvIndex_);
}

void RayTracedAOPass::UpdateParam(uint32_t frameIndex, const GBufferPass& gbuffer) {
	frameCounter_++;

	AOPassParam param{};
	param.screenWidth = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenHeight = static_cast<uint32_t>(MyWin::kWindowHeight);
	// フレーム番号をそのままシェーダへ渡すと、時間が経つほど float の精度を食って
	// ずらし量が飽和する。剰余を取るのはCPU側の倍精度で済ませる
	param.temporalJitter = static_cast<float>(
		std::fmod(static_cast<double>(frameCounter_) * kGoldenRatioConjugate, 1.0));

	*paramMapped_[frameIndex] = param;

	DenoiseTemporalParam temporal{};
	temporal.prevViewProj = prevViewProj_;
	temporal.prevInvViewProj = prevInvViewProj_;
	temporal.screenWidth = param.screenWidth;
	temporal.screenHeight = param.screenHeight;
	// 前フレームのG-Bufferが無いと履歴の妥当性を確かめられない。AOを入れ直した直後がこれに当たる
	temporal.hasHistory = (hasHistory_ && gbuffer.IsPreviousRendered()) ? 1u : 0u;
	temporal.maxHistoryLength = maxHistoryLength_;
	temporal.normalThreshold = normalThreshold_;
	temporal.positionThreshold = positionThreshold_;

	*temporalMapped_[frameIndex] = temporal;

	// 次フレームの再投影に使う
	prevViewProj_ = CameraManager::GetInstance()->GetCamera()->GetViewProjectionMatrix();
	prevInvViewProj_ = Math::Inverse(prevViewProj_);
}
