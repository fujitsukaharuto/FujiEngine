#include "Engine/Graphics/Raytracing/ScreenReflectionPass.h"

#include <algorithm>
#include <cstring>

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
#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	constexpr DXGI_FORMAT kRadianceFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// 空の像を置く距離の上限
	constexpr float kMaxRayDistance = 1000.0f;

	uint32_t DispatchCount(uint32_t count, uint32_t groupSize) {
		return (count + groupSize - 1) / groupSize;
	}
}


void ScreenReflectionPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();

	trace_.Create(device, kRadianceFormat, false);
	blurTemp_.Create(device, kRadianceFormat, false);
	result_.Create(device, kRadianceFormat, false);
	sceneColor_.Create(device, kSceneColorFormat, false);

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, kParamStride_ * kParamSlots_);
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_Trace, "  Reflection trace");
	gpuTimer_.SetName(kTimer_Blur, "  Reflection blur");
}

void ScreenReflectionPass::Finalize() {
	gpuTimer_.Finalize();
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	trace_.Reset();
	blurTemp_.Reset();
	result_.Reset();
	sceneColor_.Reset();
	hasSceneColor_ = false;
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void ScreenReflectionPass::UpdateParam(uint32_t frameIndex) {
	ReflectionPassParam param{};
	param.screenSize[0] = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenSize[1] = static_cast<uint32_t>(MyWin::kWindowHeight);
	param.reflectAll = (lightManager_->GetData().renderFlags & kRenderFlagReflectAll) != 0 ? 1u : 0u;
	param.maxRoughness = kMaxRoughness_;
	param.maxRayDistance = (std::min)(lightManager_->GetData().reflectionMaxDistance, kMaxRayDistance);
	param.pixelsPerUnit = static_cast<float>(MyWin::kWindowHeight) * 0.5f *
		CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix().m[1][1];
	param.sceneColorViewProj = sceneColorViewProj_;
	param.useSceneColor = (IsSceneColorEnabled() && hasSceneColor_) ? 1u : 0u;
	// レイ / 横 / 縦 の3つ
	for (uint32_t slot = 0; slot < kParamSlots_; slot++) {
		param.blurDirection[0] = (slot == 1) ? 1 : 0;
		param.blurDirection[1] = (slot == 2) ? 1 : 0;
		param.inputIsTrace = (slot == 1) ? 1u : 0u;
		param.stretch = (slot == 2) ? stretch_ : 0.0f;
		std::memcpy(paramMapped_[frameIndex] + kParamStride_ * slot, &param, sizeof(param));
	}
}

void ScreenReflectionPass::DispatchBlur(uint32_t frameIndex, uint32_t paramSlot, const GBufferPass& gbuffer,
	const WorkTexture& input, WorkTexture& output) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(output.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ScreenReflectionResolveCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kReflectionPassParam,
		paramResource_[frameIndex]->GetGPUVirtualAddress() + kParamStride_ * paramSlot);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kReflectionTrace, srvManager->GetGPUDescriptorHandle(trace_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kReflectionBlurInput, srvManager->GetGPUDescriptorHandle(input.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kReflectionOut, srvManager->GetGPUDescriptorHandle(output.uavIndex));
	cList->Dispatch(DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kThreadGroupSize_),
		DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kThreadGroupSize_), 1);
	dxcommon_->TransitionResource(output.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void ScreenReflectionPass::Render(const GBufferPass& gbuffer, const RaytracingScene& scene) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !gbuffer.IsRendered() ||
		(lightManager_->GetData().renderFlags & kRenderFlagScreenReflection) == 0 || !scene.IsAvailable() ||
		scene.GetTlasSrvIndex() == RaytracingScene::kInvalidSrvIndex) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	const D3D12_GPU_VIRTUAL_ADDRESS cameraInfo = CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress();
	const D3D12_GPU_VIRTUAL_ADDRESS param = paramResource_[frameIndex]->GetGPUVirtualAddress();
	UpdateParam(frameIndex);
	const uint32_t groupsX = DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kThreadGroupSize_);
	const uint32_t groupsY = DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kThreadGroupSize_);

	// 鏡の向きへ1本ずつ飛ばす
	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);
	dxcommon_->TransitionResource(trace_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ScreenReflectionTraceCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, cameraInfo);
	pPipeManager->SetComputeRootCBV(cList, RootName::kReflectionPassParam, param);
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
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kReflectionSceneColor, srvManager->GetGPUDescriptorHandle(sceneColor_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kReflectionTraceOut, srvManager->GetGPUDescriptorHandle(trace_.uavIndex));
	cList->Dispatch(groupsX, groupsY, 1);
	dxcommon_->TransitionResource(trace_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);

	// 横→縦の順にぼかす
	gpuTimer_.Begin(cList, frameIndex, kTimer_Blur);
	DispatchBlur(frameIndex, 1, gbuffer, trace_, blurTemp_);
	DispatchBlur(frameIndex, 2, gbuffer, blurTemp_, result_);
	gpuTimer_.End(cList, frameIndex, kTimer_Blur);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Blur);
}

bool ScreenReflectionPass::IsSceneColorEnabled() const {
	const uint32_t flags = lightManager_->GetData().renderFlags;
	return (flags & kRenderFlagScreenReflection) != 0 && (flags & kRenderFlagReflectionSceneColor) != 0;
}

void ScreenReflectionPass::CaptureSceneColor() {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !IsSceneColorEnabled()) {
		// 入れ直したフレームは画面の写しを使わない
		hasSceneColor_ = false;
		return;
	}
	ID3D12Resource* sceneColor = dxcommon_->GetOffscreenManager()->GetOffscreenResource(dxcommon_->GetNowFrameCount());
	dxcommon_->TransitionResource(sceneColor, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	dxcommon_->TransitionResource(sceneColor_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_COPY_DEST);
	dxcommon_->GetCommandList()->CopyResource(sceneColor_.resource.Get(), sceneColor);
	dxcommon_->TransitionResource(sceneColor_.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, WorkTexture::kReadState);
	dxcommon_->TransitionResource(sceneColor, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	sceneColorViewProj_ = CameraManager::GetInstance()->GetCamera()->GetViewProjectionMatrix();
	hasSceneColor_ = true;
}

D3D12_GPU_DESCRIPTOR_HANDLE ScreenReflectionPass::GetResultSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(result_.srvIndex);
}

void ScreenReflectionPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Reflection (screen)")) {
		return;
	}
	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("blur  : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Blur));

	static float scale = 0.25f;
	ImGui::SliderFloat("preview scale##screenreflection", &scale, 0.1f, 1.0f);
	const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);
	ImGui::Text("Trace");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(trace_.srvIndex).ptr), size);
	ImGui::Text("Result");
	ImGui::Image(static_cast<ImTextureID>(GetResultSrvHandle().ptr), size);
	ImGui::TreePop();
#endif // _DEBUGMODE
}
