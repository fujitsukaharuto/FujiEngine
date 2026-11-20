#include "Engine/Graphics/Raytracing/PunctualShadowPass.h"

#include <algorithm>
#include <cmath>
#include <vector>

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

	// rgb が枠 0〜2 の遮蔽率。R8 だと半影に段差が見えるので FP16
	constexpr DXGI_FORMAT kShadowFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	constexpr double kGoldenRatioConjugate = 0.6180339887498949;

	constexpr uint32_t kThreadGroupSize = 8;

	uint32_t DispatchCount(int32_t pixels) {
		return (static_cast<uint32_t>(pixels) + kThreadGroupSize - 1) / kThreadGroupSize;
	}

	uint32_t SlotKey(uint32_t slots, uint32_t slot) {
		return (slots >> (slot * 8u)) & 0xFFu;
	}

	/// <summary>抜きのある物の影がアルファを引くための見た目 (RayTracedShadowPass と同じ)</summary>
	void BindRayCutout(ID3D12GraphicsCommandList* cList) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		SRVManager* srvManager = SRVManager::GetInstance();
		const RaytracingScene* scene = ObjectRenderer::GetInstance()->GetRaytracingScene();
		const uint32_t instanceSrv = scene ? scene->GetInstanceDataSrvIndex() : RaytracingScene::kInvalidSrvIndex;
		if (instanceSrv == RaytracingScene::kInvalidSrvIndex) { return; }
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayInstances, srvManager->GetGPUDescriptorHandle(instanceSrv));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayVertices, srvManager->GetGPUDescriptorHandle(0));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayIndices, srvManager->GetGPUDescriptorHandle(0));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayTextures, srvManager->GetGPUDescriptorHandle(0));
	}
}


void PunctualShadowPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;

	ID3D12Device* device = dxcommon_->GetDevice();

	raw_.Create(device, kShadowFormat, false);
	denoiser_.Initialize(pDxcom, true);
	// 平行光源の影と同じく、動く物の影に追従させる
	denoiser_.SetHistoryClamp(true);

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(PunctualShadowParam));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), 1);
	gpuTimer_.SetName(kTimer_Trace, "  Local shadow trace");
	denoiser_.SetTimerNames("  Local shadow temporal", "  Local shadow spatial");
}

void PunctualShadowPass::Finalize() {
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

void PunctualShadowPass::Prepare() {
	if (dxcommon_ == nullptr || lightManager_ == nullptr) {
		return;
	}
	const uint32_t slots = SelectLights();
	// 枠の中身が別の光源に変わったら、その枠の蓄積は別の影
	for (uint32_t slot = 0; slot < kSoftShadowSlotCount; slot++) {
		if (SlotKey(slots, slot) != SlotKey(slots_, slot) && SlotKey(slots_, slot) != kSoftShadowSlotEmpty) {
			denoiser_.ResetHistory();
			break;
		}
	}
	slots_ = slots;
	slotCount_ = 0;
	for (uint32_t slot = 0; slot < kSoftShadowSlotCount; slot++) {
		slotCount_ += (SlotKey(slots_, slot) != kSoftShadowSlotEmpty) ? 1u : 0u;
	}
	lightManager_->SetSoftShadowLightsThisFrame(slots_);
}

uint32_t PunctualShadowPass::SelectLights() const {
	const AllLightsData& lights = lightManager_->GetUploadedData();
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	if ((lights.renderFlags & kRenderFlagSoftLocalShadow) == 0 || camera == nullptr) {
		return kSoftShadowSlotsNone;
	}
	const Math::Vector3 cameraPos = camera->GetTranslate();
	const Math::Matrix4x4& world = camera->GetWorldMatrix();
	const Math::Vector3 forward = { world.m[2][0], world.m[2][1], world.m[2][2] };

	struct Candidate {
		uint32_t key;
		float distance;
	};
	std::vector<Candidate> candidates;
	// 光の届く球がカメラの後ろに丸ごとある光源は外し、球の手前の端が近い順に選ぶ
	auto consider = [&](uint32_t key, const Math::Vector4& color, float intensity, const Math::Vector3& position,
		float range, float sourceRadius) {
		const bool isLit = color.x * intensity > 0.0f || color.y * intensity > 0.0f || color.z * intensity > 0.0f;
		if (!isLit || range <= 0.0f || sourceRadius <= 0.0f) {
			return;
		}
		const Math::Vector3 toLight = position - cameraPos;
		if (Math::Vector3::Dot(toLight, forward) < -range * forward.Length()) {
			return;
		}
		candidates.push_back({ key, (std::max)(toLight.Length() - range, 0.0f) });
	};
	if ((lights.rayTracedShadowMask & kShadowMaskPoint) != 0) {
		for (int32_t i = 0; i < lights.numPointLights && i < kMaxPointLights; ++i) {
			const PointLightData& light = lights.pointLights[i];
			consider(static_cast<uint32_t>(i), light.color, light.intensity, light.position, light.radius, light.sourceRadius);
		}
	}
	if ((lights.rayTracedShadowMask & kShadowMaskSpot) != 0) {
		for (int32_t i = 0; i < lights.numSpotLights && i < kMaxSpotLights; ++i) {
			const SpotLightData& light = lights.spotLights[i];
			consider(static_cast<uint32_t>(i) | kSoftShadowSlotSpot, light.color, light.intensity, light.position, light.distance, light.sourceRadius);
		}
	}
	std::stable_sort(candidates.begin(), candidates.end(),
		[](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
	if (candidates.size() > kSoftShadowSlotCount) {
		candidates.resize(kSoftShadowSlotCount);
	}

	// 前のフレームと同じ光源は同じ枠へ。空いた枠に新しい光源を詰める
	uint32_t keys[kSoftShadowSlotCount] = { kSoftShadowSlotEmpty, kSoftShadowSlotEmpty, kSoftShadowSlotEmpty };
	std::vector<uint32_t> newcomers;
	for (const Candidate& candidate : candidates) {
		bool isKept = false;
		for (uint32_t slot = 0; slot < kSoftShadowSlotCount; slot++) {
			if (SlotKey(slots_, slot) == candidate.key) {
				keys[slot] = candidate.key;
				isKept = true;
				break;
			}
		}
		if (!isKept) {
			newcomers.push_back(candidate.key);
		}
	}
	for (uint32_t key : newcomers) {
		for (uint32_t slot = 0; slot < kSoftShadowSlotCount; slot++) {
			if (keys[slot] == kSoftShadowSlotEmpty) {
				keys[slot] = key;
				break;
			}
		}
	}
	uint32_t slots = kSoftShadowSlotsNone;
	for (uint32_t slot = 0; slot < kSoftShadowSlotCount; slot++) {
		slots = (slots & ~(0xFFu << (slot * 8u))) | (keys[slot] << (slot * 8u));
	}
	return slots;
}

void PunctualShadowPass::Disable() {
	if (slots_ != kSoftShadowSlotsNone && lightManager_) {
		lightManager_->SetSoftShadowLightsThisFrame(kSoftShadowSlotsNone);
	}
	slots_ = kSoftShadowSlotsNone;
	slotCount_ = 0;
	denoiser_.ResetHistory();
}

void PunctualShadowPass::Render(const GBufferPass& gbuffer) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr || slotCount_ == 0 || !gbuffer.IsRendered()) {
		Disable();
		return;
	}
	auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene();
	const uint32_t tlasSrv = (rtScene && rtScene->IsAvailable()) ? rtScene->GetTlasSrvIndex() : RaytracingScene::kInvalidSrvIndex;
	if (tlasSrv == RaytracingScene::kInvalidSrvIndex) {
		Disable();
		return;
	}

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	UpdateParam(frameIndex);
	DispatchTrace(frameIndex, gbuffer, tlasSrv);
	denoiser_.Render(gbuffer, raw_);
}

void PunctualShadowPass::DispatchTrace(uint32_t frameIndex, const GBufferPass& gbuffer, uint32_t tlasSrvIndex) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(raw_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);

	pPipeManager->SetCSPipeline(Pipe::PunctualShadowCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kCameraInfo,
		CameraManager::GetInstance()->GetCamera()->GetCameraInfoGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kPunctualShadowParam, paramResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferNormal, gbuffer.GetNormalSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kGBufferDepth, gbuffer.GetDepthSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(tlasSrvIndex));
	BindRayCutout(cList);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kPunctualShadowOutput, srvManager->GetGPUDescriptorHandle(raw_.uavIndex));

	cList->Dispatch(DispatchCount(MyWin::kWindowWidth), DispatchCount(MyWin::kWindowHeight), 1);

	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);
	dxcommon_->TransitionResource(raw_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void PunctualShadowPass::UpdateParam(uint32_t frameIndex) {
	frameCounter_++;

	PunctualShadowParam param{};
	param.screenWidth = static_cast<uint32_t>(MyWin::kWindowWidth);
	param.screenHeight = static_cast<uint32_t>(MyWin::kWindowHeight);
	param.temporalJitter = static_cast<float>(
		std::fmod(static_cast<double>(frameCounter_) * kGoldenRatioConjugate, 1.0));
	*paramMapped_[frameIndex] = param;
}

void PunctualShadowPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Local soft shadow (screen space)")) {
		return;
	}

	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace    : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("temporal : %.3f ms", denoiser_.GetTemporalMS());
	ImGui::Text("spatial  : %.3f ms", denoiser_.GetSpatialMS());
	for (uint32_t slot = 0; slot < kSoftShadowSlotCount; slot++) {
		const uint32_t key = SlotKey(slots_, slot);
		if (key == kSoftShadowSlotEmpty) {
			ImGui::Text("slot %u : -", slot);
		} else {
			ImGui::Text("slot %u : %s %u", slot, (key & kSoftShadowSlotSpot) ? "spot" : "point", key & ~kSoftShadowSlotSpot);
		}
	}

	denoiser_.DebugGUI();

	static float scale = 0.25f;
	ImGui::SliderFloat("preview scale##localshadow", &scale, 0.1f, 1.0f);
	const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);
	ImGui::Text("Raw");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(raw_.srvIndex).ptr), size);
	ImGui::Text("Denoised");
	ImGui::Image(static_cast<ImTextureID>(GetShadowSrvHandle().ptr), size);

	ImGui::TreePop();
#endif // _DEBUGMODE
}
