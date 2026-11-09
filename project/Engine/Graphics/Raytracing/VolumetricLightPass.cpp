#include "Engine/Graphics/Raytracing/VolumetricLightPass.h"

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/Light/ClusteredLightPass.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	constexpr DXGI_FORMAT kLightFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	uint32_t DispatchCount(uint32_t count, uint32_t groupSize) {
		return (count + groupSize - 1) / groupSize;
	}
}


void VolumetricLightPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();

	trace_.Create(device, kLightFormat, false, GetWidth(), GetHeight());
	result_.Create(device, kLightFormat, false, GetWidth(), GetHeight());

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(VolumetricParam));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_Trace, "  Volumetric trace");
	gpuTimer_.SetName(kTimer_Blur, "  Volumetric blur");
}

void VolumetricLightPass::Finalize() {
	gpuTimer_.Finalize();
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	trace_.Reset();
	result_.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

uint32_t VolumetricLightPass::GetWidth() const {
	return static_cast<uint32_t>(MyWin::kWindowWidth) / kDownscale_;
}

uint32_t VolumetricLightPass::GetHeight() const {
	return static_cast<uint32_t>(MyWin::kWindowHeight) / kDownscale_;
}

void VolumetricLightPass::UpdateParam(uint32_t frameIndex) {
	VolumetricParam param{};
	param.lowSize[0] = GetWidth();
	param.lowSize[1] = GetHeight();
	param.screenSize[0] = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenSize[1] = static_cast<uint32_t>(MyWin::kWindowHeight);
	param.density = settings_.density;
	param.anisotropy = settings_.anisotropy;
	param.maxDistance = settings_.maxDistance;
	param.intensity = settings_.intensity;
	param.sunScale = settings_.sunScale;
	param.localLightScale = settings_.localLightScale;
	// 透視投影の深度は d = m22 + m32 / z
	const Math::Matrix4x4& projection = CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix();
	param.depthToView[0] = projection.m[2][2];
	param.depthToView[1] = projection.m[3][2];
	*paramMapped_[frameIndex] = param;
}

void VolumetricLightPass::Render(const GBufferPass& gbuffer, const RaytracingScene& scene, const ClusteredLightPass& clustered) {
	isRendered_ = false;
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !IsEnabled() || !gbuffer.IsRendered() ||
		!scene.IsAvailable() || scene.GetTlasSrvIndex() == RaytracingScene::kInvalidSrvIndex) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateParam(frameIndex);
	const D3D12_GPU_VIRTUAL_ADDRESS param = paramResource_[frameIndex]->GetGPUVirtualAddress();
	const uint32_t groupsX = DispatchCount(GetWidth(), kThreadGroupSize_);
	const uint32_t groupsY = DispatchCount(GetHeight(), kThreadGroupSize_);

	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);
	dxcommon_->TransitionResource(trace_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::VolumetricLightTraceCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kVolumetricParam, param);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene.GetTlasSrvIndex()));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricTraceOut, srvManager->GetGPUDescriptorHandle(trace_.uavIndex));
	pPipeManager->SetComputeRootCBV(cList, RootName::kClusterParams, clustered.GetParamsGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusteredLights, clustered.GetLightsSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusterCounts, clustered.GetCountsSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusterIndices, clustered.GetIndicesSrvHandle());
	cList->Dispatch(groupsX, groupsY, 1);
	dxcommon_->TransitionResource(trace_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);

	// 進み始めのずらしの模様を均す
	gpuTimer_.Begin(cList, frameIndex, kTimer_Blur);
	dxcommon_->TransitionResource(result_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::VolumetricLightBlurCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kVolumetricParam, param);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricBlurInput, srvManager->GetGPUDescriptorHandle(trace_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricOut, srvManager->GetGPUDescriptorHandle(result_.uavIndex));
	cList->Dispatch(groupsX, groupsY, 1);
	dxcommon_->TransitionResource(result_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Blur);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Blur);

	isRendered_ = true;
}

D3D12_GPU_DESCRIPTOR_HANDLE VolumetricLightPass::GetResultSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(result_.srvIndex);
}

void VolumetricLightPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Volumetric light")) {
		return;
	}
	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("blur  : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Blur));
	const ImVec2 size(static_cast<float>(GetWidth()), static_cast<float>(GetHeight()));
	ImGui::Image(static_cast<ImTextureID>(GetResultSrvHandle().ptr), size);
	ImGui::TreePop();
#endif // _DEBUGMODE
}
