#include "Engine/Graphics/Raytracing/ScreenProbeGIPass.h"

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/IBL/IBLBaker.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	// 係数と位置は FP32、法線と間接光は FP16
	constexpr DXGI_FORMAT kProbeSHFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr DXGI_FORMAT kProbePositionFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	constexpr DXGI_FORMAT kProbeNormalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr DXGI_FORMAT kIrradianceFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr DXGI_FORMAT kCountFormat = DXGI_FORMAT_R16_FLOAT;

	uint32_t DispatchCount(uint32_t count, uint32_t groupSize) {
		return (count + groupSize - 1) / groupSize;
	}
}


void ScreenProbeGIPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();

	probeCountX_ = DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kTileSize_);
	probeCountY_ = DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kTileSize_);

	probeSH_.Create(device, kProbeSHFormat, false, probeCountX_ * 3, probeCountY_);
	probeSHFiltered_.Create(device, kProbeSHFormat, false, probeCountX_ * 3, probeCountY_);
	probePosition_.Create(device, kProbePositionFormat, false, probeCountX_, probeCountY_);
	probeNormal_.Create(device, kProbeNormalFormat, false, probeCountX_, probeCountY_);
	raw_.Create(device, kIrradianceFormat, false);
	for (uint32_t i = 0; i < 2; i++) {
		history_[i].Create(device, kIrradianceFormat, false);
		historyCount_[i].Create(device, kCountFormat, false);
	}

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(ScreenProbeParam));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_Trace, "  Screen GI trace");
	gpuTimer_.SetName(kTimer_Resolve, "  Screen GI resolve");
}

void ScreenProbeGIPass::Finalize() {
	gpuTimer_.Finalize();
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	probeSH_.Reset();
	probeSHFiltered_.Reset();
	probePosition_.Reset();
	probeNormal_.Reset();
	raw_.Reset();
	for (uint32_t i = 0; i < 2; i++) {
		history_[i].Reset();
		historyCount_[i].Reset();
	}
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void ScreenProbeGIPass::UpdateParam(uint32_t frameIndex) {
	Camera* camera = CameraManager::GetInstance()->GetCamera();

	ScreenProbeParam param{};
	param.prevViewProj = prevViewProj_;
	param.prevInvViewProj = prevInvViewProj_;
	param.screenSize[0] = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenSize[1] = static_cast<uint32_t>(MyWin::kWindowHeight);
	param.probeCount[0] = probeCountX_;
	param.probeCount[1] = probeCountY_;
	param.tileSize = kTileSize_;
	param.frameIndex = frameCounter_++;
	param.hasHistory = hasHistory_ ? 1u : 0u;
	param.maxHistory = kMaxHistory_;
	param.maxRayDistance = kMaxRayDistance_;
	param.pixelsPerUnit = static_cast<float>(MyWin::kWindowHeight) * 0.5f * camera->GetProjectionMatrix().m[1][1];
	*paramMapped_[frameIndex] = param;

	prevViewProj_ = camera->GetViewProjectionMatrix();
	prevInvViewProj_ = Math::Inverse(prevViewProj_);
}

void ScreenProbeGIPass::Render(const GBufferPass& gbuffer, const RaytracingScene& scene) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !gbuffer.IsRendered() ||
		(lightManager_->GetData().renderFlags & kRenderFlagScreenGI) == 0 || !scene.IsAvailable() ||
		scene.GetTlasSrvIndex() == RaytracingScene::kInvalidSrvIndex) {
		hasHistory_ = false;
		return;
	}
	// プリパスが前のフレームに無ければ前の深度と法線は古い
	hasHistory_ = hasHistory_ && gbuffer.IsPreviousRendered();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	const D3D12_GPU_VIRTUAL_ADDRESS cameraInfo = CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress();
	const D3D12_GPU_VIRTUAL_ADDRESS param = paramResource_[frameIndex]->GetGPUVirtualAddress();
	UpdateParam(frameIndex);

	auto toUav = [this](WorkTexture& texture) {
		dxcommon_->TransitionResource(texture.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	};
	auto toRead = [this](WorkTexture& texture) {
		dxcommon_->TransitionResource(texture.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
	};
	auto srv = [srvManager](const WorkTexture& texture) { return srvManager->GetGPUDescriptorHandle(texture.srvIndex); };
	auto uav = [srvManager](const WorkTexture& texture) { return srvManager->GetGPUDescriptorHandle(texture.uavIndex); };

	// プローブを置いてレイを飛ばす
	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);
	toUav(probeSH_);
	toUav(probePosition_);
	toUav(probeNormal_);
	pPipeManager->SetCSPipeline(Pipe::ScreenProbeTraceCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, cameraInfo);
	pPipeManager->SetComputeRootCBV(cList, RootName::kScreenProbeParam, param);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene.GetTlasSrvIndex()));
	const uint32_t instanceSrv = scene.GetInstanceDataSrvIndex();
	if (instanceSrv != RaytracingScene::kInvalidSrvIndex) {
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayInstances, srvManager->GetGPUDescriptorHandle(instanceSrv));
	}
	// 頂点・インデックス・テクスチャはヒープの先頭を渡し、番号で引く
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayVertices, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayIndices, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kTextures, srvManager->GetGPUDescriptorHandle(0));
	if (IBLBaker* ibl = ObjectRenderer::GetInstance()->GetIBLBaker()) {
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kPrefilteredEnv, ibl->GetPrefilteredSrvHandle());
	}
	if (DDGIPass* ddgi = ObjectRenderer::GetInstance()->GetDDGIPass()) {
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolumes, ddgi->GetVolumesGPUVirtualAddress());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIIrradiance, ddgi->GetIrradianceSrvHandle());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIDistance, ddgi->GetDistanceSrvHandle());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIProbeData, ddgi->GetProbeDataSrvHandle());
	}
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeSHOut, uav(probeSH_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbePositionOut, uav(probePosition_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeNormalOut, uav(probeNormal_));
	cList->Dispatch(probeCountX_, probeCountY_, 1);
	toRead(probeSH_);
	toRead(probePosition_);
	toRead(probeNormal_);
	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);

	gpuTimer_.Begin(cList, frameIndex, kTimer_Resolve);

	// 隣のプローブと何回か混ぜ、最後は probeSHFiltered_ に残す
	pPipeManager->SetCSPipeline(Pipe::ScreenProbeFilterCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kScreenProbeParam, param);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbePosition, srv(probePosition_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeNormal, srv(probeNormal_));
	static_assert(kFilterPasses_ % 2 == 1, "最後の書き先が probeSHFiltered_ になるよう奇数回にする");
	for (uint32_t pass = 0; pass < kFilterPasses_; pass++) {
		WorkTexture& source = (pass % 2 == 0) ? probeSH_ : probeSHFiltered_;
		WorkTexture& destination = (pass % 2 == 0) ? probeSHFiltered_ : probeSH_;
		toUav(destination);
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeSH, srv(source));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeSHFilteredOut, uav(destination));
		cList->Dispatch(DispatchCount(probeCountX_, kThreadGroupSize_), DispatchCount(probeCountY_, kThreadGroupSize_), 1);
		toRead(destination);
	}

	const uint32_t screenGroupsX = DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kThreadGroupSize_);
	const uint32_t screenGroupsY = DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kThreadGroupSize_);

	// 画素ごとに近くのプローブから引く
	toUav(raw_);
	pPipeManager->SetCSPipeline(Pipe::ScreenProbeIntegrateCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, cameraInfo);
	pPipeManager->SetComputeRootCBV(cList, RootName::kScreenProbeParam, param);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeSHFiltered, srv(probeSHFiltered_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbePosition, srv(probePosition_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeNormal, srv(probeNormal_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kScreenGIRawOut, uav(raw_));
	cList->Dispatch(screenGroupsX, screenGroupsY, 1);
	toRead(raw_);

	// 前のフレームの結果と混ぜる
	const uint32_t previous = current_;
	current_ ^= 1u;
	toUav(history_[current_]);
	toUav(historyCount_[current_]);
	pPipeManager->SetCSPipeline(Pipe::ScreenProbeTemporalCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, cameraInfo);
	pPipeManager->SetComputeRootCBV(cList, RootName::kScreenProbeParam, param);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormalPrev, gbuffer.GetNormalSrvHandle(true));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepthPrev, gbuffer.GetDepthSrvHandle(true));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kScreenGIRaw, srv(raw_));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kScreenGIHistoryPrev, srv(history_[previous]));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kScreenGICountPrev, srv(historyCount_[previous]));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kScreenGIOut, uav(history_[current_]));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kScreenGICountOut, uav(historyCount_[current_]));
	cList->Dispatch(screenGroupsX, screenGroupsY, 1);
	toRead(history_[current_]);
	toRead(historyCount_[current_]);

	gpuTimer_.End(cList, frameIndex, kTimer_Resolve);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Resolve);

	hasHistory_ = true;
}

D3D12_GPU_DESCRIPTOR_HANDLE ScreenProbeGIPass::GetResultSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(history_[current_].srvIndex);
}

void ScreenProbeGIPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("GI (screen probes)")) {
		return;
	}
	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace   : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("resolve : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Resolve));
	ImGui::Text("probes  : %u x %u, %u rays", probeCountX_, probeCountY_, kRaysPerProbe_);

	static float scale = 0.25f;
	ImGui::SliderFloat("preview scale##screengi", &scale, 0.1f, 1.0f);
	const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);
	ImGui::Text("Raw");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(raw_.srvIndex).ptr), size);
	ImGui::Text("Result");
	ImGui::Image(static_cast<ImTextureID>(GetResultSrvHandle().ptr), size);
	ImGui::TreePop();
#endif // _DEBUGMODE
}
