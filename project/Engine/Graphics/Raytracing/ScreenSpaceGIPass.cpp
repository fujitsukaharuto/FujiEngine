#include "Engine/Graphics/Raytracing/ScreenSpaceGIPass.h"

#include <cstring>

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
#include "Engine/Graphics/Raytracing/ScreenReflectionPass.h"
#include "Engine/Graphics/SkyBox/SkyAtmosphere.h"
#include "Engine/Graphics/Raytracing/VolumetricFogPass.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	constexpr DXGI_FORMAT kResultFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	uint32_t DispatchCount(uint32_t count, uint32_t groupSize) {
		return (count + groupSize - 1) / groupSize;
	}
}


void ScreenSpaceGIPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();

	trace_.Create(device, kResultFormat, false);
	blurTemp_.Create(device, kResultFormat, false);
	result_.Create(device, kResultFormat, false);

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, kParamStride_ * kParamSlots_);
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_Trace, "  SSGI trace");
	gpuTimer_.SetName(kTimer_Blur, "  SSGI blur");
}

void ScreenSpaceGIPass::Finalize() {
	gpuTimer_.Finalize();
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	trace_.Reset();
	blurTemp_.Reset();
	result_.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void ScreenSpaceGIPass::UpdateParam(uint32_t frameIndex, const ScreenReflectionPass& sceneColor) {
	const Math::Matrix4x4& projection = CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix();
	SSGIPassParam param{};
	param.sceneColorViewProj = sceneColor.GetSceneColorViewProj();
	param.screenSize[0] = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenSize[1] = static_cast<uint32_t>(MyWin::kWindowHeight);
	param.maxDistance = settings_.maxDistance;
	param.intensity = settings_.intensity;
	param.useSceneColor = sceneColor.HasSceneColor() ? 1u : 0u;
	param.depthToViewZ[0] = projection.m[3][2];
	param.depthToViewZ[1] = projection.m[2][2];
	// レイ / 模様の横・縦 / 広い横・縦
	for (uint32_t slot = 0; slot < kParamSlots_; slot++) {
		param.blurDirection[0] = (slot == 1 || slot == 3) ? 1 : 0;
		param.blurDirection[1] = (slot == 2 || slot == 4) ? 1 : 0;
		param.blurWide = (slot >= 3) ? 1u : 0u;
		std::memcpy(paramMapped_[frameIndex] + kParamStride_ * slot, &param, sizeof(param));
	}
}

void ScreenSpaceGIPass::DispatchBlur(uint32_t frameIndex, uint32_t paramSlot, const GBufferPass& gbuffer,
	const WorkTexture& input, WorkTexture& output) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(output.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ScreenSpaceGIBlurCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kSSGIPassParam,
		paramResource_[frameIndex]->GetGPUVirtualAddress() + kParamStride_ * paramSlot);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSSGIBlurInput, srvManager->GetGPUDescriptorHandle(input.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSSGIOut, srvManager->GetGPUDescriptorHandle(output.uavIndex));
	cList->Dispatch(DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kThreadGroupSize_),
		DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kThreadGroupSize_), 1);
	dxcommon_->TransitionResource(output.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void ScreenSpaceGIPass::Render(const GBufferPass& gbuffer, const ScreenReflectionPass& sceneColor) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || !gbuffer.IsRendered() ||
		(lightManager_->GetData().renderFlags & kRenderFlagScreenSpaceGI) == 0) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateParam(frameIndex, sceneColor);

	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);
	dxcommon_->TransitionResource(trace_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ScreenSpaceGITraceCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo, CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kSSGIPassParam, paramResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSSGISceneColor, sceneColor.GetSceneColorSrvHandle());
	// 画面の色から前方描画と同じ空気遠近を外す
	if (const SkyAtmosphere* atmosphere = ObjectRenderer::GetInstance()->GetSkyAtmosphere()) {
		pPipeManager->SetComputeRootCBV(cList, RootName::kAerialPerspective, atmosphere->GetAerialPerspectiveParamsAddress());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAerialScatter, atmosphere->GetAerialScatterSrvHandle());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kAerialTransmittance, atmosphere->GetAerialTransmittanceSrvHandle());
	}
	// ボリュメトリックフォグも同じく外す
	if (const VolumetricFogPass* fog = ObjectRenderer::GetInstance()->GetVolumetricFogPass()) {
		pPipeManager->SetComputeRootCBV(cList, RootName::kVolumetricFog, fog->GetParamsGPUVirtualAddress());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricFogVolume, fog->GetVolumeSrvHandle());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kLocalFogVolumes, fog->GetLocalFogVolumesSrvHandle());
	}
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSSGITraceOut, srvManager->GetGPUDescriptorHandle(trace_.uavIndex));
	cList->Dispatch(DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kThreadGroupSize_),
		DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kThreadGroupSize_), 1);
	dxcommon_->TransitionResource(trace_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);

	// 模様を消してから広くならす。どちらも横→縦
	gpuTimer_.Begin(cList, frameIndex, kTimer_Blur);
	DispatchBlur(frameIndex, 1, gbuffer, trace_, blurTemp_);
	DispatchBlur(frameIndex, 2, gbuffer, blurTemp_, result_);
	DispatchBlur(frameIndex, 3, gbuffer, result_, blurTemp_);
	DispatchBlur(frameIndex, 4, gbuffer, blurTemp_, result_);
	gpuTimer_.End(cList, frameIndex, kTimer_Blur);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Blur);
}

D3D12_GPU_DESCRIPTOR_HANDLE ScreenSpaceGIPass::GetResultSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(result_.srvIndex);
}

void ScreenSpaceGIPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("SSGI")) {
		return;
	}
	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("blur  : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Blur));

	static float scale = 0.25f;
	ImGui::SliderFloat("preview scale##ssgi", &scale, 0.1f, 1.0f);
	const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);
	ImGui::Text("Trace");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(trace_.srvIndex).ptr), size);
	ImGui::Text("Result");
	ImGui::Image(static_cast<ImTextureID>(GetResultSrvHandle().ptr), size);
	ImGui::TreePop();
#endif // _DEBUGMODE
}
