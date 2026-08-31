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

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	// AOは0〜1の1チャンネル。R8だと平坦な階調に段差が見えるのでFP16
	constexpr DXGI_FORMAT kAOFormat = DXGI_FORMAT_R16_FLOAT;

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

	raw_.Create(device, kAOFormat, false);
	denoiser_.Initialize(pDxcom);

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(AOPassParam));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), 1);
}

void RayTracedAOPass::Finalize() {
	gpuTimer_.Finalize();
	denoiser_.Finalize();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	raw_.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void RayTracedAOPass::Render(const GBufferPass& gbuffer) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !gbuffer.IsRendered() ||
		lightManager_->GetData().aoMode != kAOModeScreen) {
		denoiser_.ResetHistory();
		return;
	}

	auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene();
	if (rtScene == nullptr || !rtScene->IsAvailable()) {
		denoiser_.ResetHistory();
		return;
	}
	const uint32_t tlasSrv = rtScene->GetTlasSrvIndex();
	if (tlasSrv == RaytracingScene::kInvalidSrvIndex) {
		denoiser_.ResetHistory();
		return;
	}

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateParam(frameIndex);

	DispatchTrace(frameIndex, gbuffer, tlasSrv);
	denoiser_.Render(gbuffer, raw_);
}

void RayTracedAOPass::DispatchTrace(uint32_t frameIndex, const GBufferPass& gbuffer, uint32_t tlasSrvIndex) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(raw_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
	dxcommon_->TransitionResource(raw_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
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
	ImGui::Text("temporal : %.3f ms", denoiser_.GetTemporalMS());
	ImGui::Text("spatial  : %.3f ms", denoiser_.GetSpatialMS());

	denoiser_.DebugGUI();

	static float scale = 0.25f;
	ImGui::SliderFloat("preview scale##ao", &scale, 0.1f, 1.0f);
	const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);

	ImGui::Text("Raw");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(raw_.srvIndex).ptr), size);
	ImGui::Text("Denoised");
	ImGui::Image(static_cast<ImTextureID>(GetAOSrvHandle().ptr), size);

	ImGui::TreePop();
#endif // _DEBUGMODE
}

void RayTracedAOPass::UpdateParam(uint32_t frameIndex) {
	frameCounter_++;

	AOPassParam param{};
	param.screenWidth = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenHeight = static_cast<uint32_t>(MyWin::kWindowHeight);
	// フレーム番号をそのままシェーダへ渡すと、時間が経つほど float の精度を食って
	// ずらし量が飽和する。剰余を取るのはCPU側の倍精度で済ませる
	param.temporalJitter = static_cast<float>(
		std::fmod(static_cast<double>(frameCounter_) * kGoldenRatioConjugate, 1.0));

	*paramMapped_[frameIndex] = param;
}
