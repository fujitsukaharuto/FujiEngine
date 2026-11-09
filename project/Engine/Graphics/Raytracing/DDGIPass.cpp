#include "Engine/Graphics/Raytracing/DDGIPass.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/IBL/IBLBaker.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace DXC;
using namespace Graphics;

namespace {

	// レイの結果。放射輝度と距離(裏面は負)
	constexpr DXGI_FORMAT kRayDataFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// 放射照度と、使えるプローブか(a)
	constexpr DXGI_FORMAT kIrradianceFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// 距離の平均と2乗の平均
	constexpr DXGI_FORMAT kDistanceFormat = DXGI_FORMAT_R16G16_FLOAT;
	// 再配置でずらした量
	constexpr DXGI_FORMAT kProbeDataFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	int32_t FloorDiv(float value, float spacing) {
		return static_cast<int32_t>(std::floor(value / spacing));
	}
}


void DDGIPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();

	// x と y を横、z を縦に並べ、格子ごとに縦へ積む
	uint32_t tilesX = 0;
	uint32_t tilesY = 0;
	uint32_t totalProbes = 0;
	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		DDGIVolumeData& volume = volumes_[v];
		for (int i = 0; i < 3; i++) {
			volume.probeCounts[i] = kProbeCounts_[v][i];
		}
		volume.firstProbe = totalProbes;
		volume.firstTileRow = tilesY;
		tilesX = (std::max)(tilesX, static_cast<uint32_t>(kProbeCounts_[v][0] * kProbeCounts_[v][1]));
		tilesY += static_cast<uint32_t>(kProbeCounts_[v][2]);
		totalProbes += ProbeCount(v);
	}
	rayData_ = CreateAtlas(device, kFixedRays_ + kMaxRaysPerProbe_, totalProbes, kRayDataFormat);
	probeData_ = CreateAtlas(device, tilesX, tilesY, kProbeDataFormat);
	irradiance_ = CreateAtlas(device, tilesX * (kIrradianceTexels_ + 2), tilesY * (kIrradianceTexels_ + 2), kIrradianceFormat);
	distance_ = CreateAtlas(device, tilesX * (kDistanceTexels_ + 2), tilesY * (kDistanceTexels_ + 2), kDistanceFormat);

	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		DDGIVolumeData& volume = volumes_[v];
		volume.irradianceAtlasSize[0] = static_cast<float>(irradiance_.width);
		volume.irradianceAtlasSize[1] = static_cast<float>(irradiance_.height);
		volume.distanceAtlasSize[0] = static_cast<float>(distance_.width);
		volume.distanceAtlasSize[1] = static_cast<float>(distance_.height);
	}

	volumeResource_ = Helper::CreateBufferResource(device, kFrameConstantStride_ * kFrameCount_);
	volumeResource_->Map(0, nullptr, reinterpret_cast<void**>(&volumeMapped_));
	// 前方描画は切っていても定数を読むので中身を入れておく
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		uint8_t* frame = volumeMapped_ + kFrameConstantStride_ * i;
		for (uint32_t v = 0; v < kVolumeCount_; v++) {
			std::memcpy(frame + kVolumeStride_ * v, &volumes_[v], sizeof(DDGIVolumeData));
			std::memcpy(frame + kVolumesOffset_ + sizeof(DDGIVolumeData) * v, &volumes_[v], sizeof(DDGIVolumeData));
		}
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_Trace, "  GI trace");
	gpuTimer_.SetName(kTimer_Update, "  GI update");
}

DDGIPass::AtlasTexture DDGIPass::CreateAtlas(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format) {
	SRVManager* srvManager = SRVManager::GetInstance();

	AtlasTexture atlas;
	atlas.width = width;
	atlas.height = height;
	atlas.resource = Helper::CreateTexture2D(device, static_cast<int32_t>(width), static_cast<int32_t>(height), format,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState, nullptr);
	atlas.srvIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(atlas.srvIndex, atlas.resource.Get(), format, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	atlas.uavIndex = srvManager->Allocate();
	device->CreateUnorderedAccessView(atlas.resource.Get(), nullptr, &uavDesc, srvManager->GetCPUDescriptorHandle(atlas.uavIndex));
	return atlas;
}

void DDGIPass::Finalize() {
	gpuTimer_.Finalize();
	if (volumeResource_) {
		volumeMapped_ = nullptr;
		volumeResource_.Reset();
	}
	rayData_.resource.Reset();
	irradiance_.resource.Reset();
	distance_.resource.Reset();
	probeData_.resource.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void DDGIPass::SetSettings(const Settings& settings) {
	Settings clamped = settings;
	clamped.probeSpacing = (std::max)(clamped.probeSpacing, 0.25f);
	// スレッドグループの幅の倍数にそろえる
	clamped.raysPerProbe = std::clamp((clamped.raysPerProbe + kRayThreadGroup_ - 1) / kRayThreadGroup_ * kRayThreadGroup_,
		kRayThreadGroup_, kMaxRaysPerProbe_);
	if (clamped.probeSpacing != settings_.probeSpacing || clamped.raysPerProbe != settings_.raysPerProbe) {
		needsReset_ = true;
	}
	settings_ = clamped;
}

void DDGIPass::UpdateVolume(uint32_t volumeIndex, uint32_t frameIndex) {
	DDGIVolumeData& volume = volumes_[volumeIndex];
	bool& hasBaseCell = hasBaseCell_[volumeIndex];
	const float spacing = settings_.probeSpacing * kSpacingRatios_[volumeIndex];
	int32_t baseCell[3] = {};
	if (Camera* camera = CameraManager::GetInstance()->GetCamera()) {
		// 格子の真ん中をカメラの前方 (水平) へずらす
		const Math::Matrix4x4& world = camera->GetWorldMatrix();
		Math::Vector3 forward = { world.m[2][0], 0.0f, world.m[2][2] };
		const float forwardLength = forward.Length();
		forward = (forwardLength > 1e-4f) ? forward / forwardLength : Math::Vector3{ 0.0f, 0.0f, 1.0f };
		const int32_t* counts = kProbeCounts_[volumeIndex];
		const float shift = static_cast<float>((std::min)(counts[0], counts[2])) * spacing * kForwardShift_;
		const Math::Vector3 center = camera->GetTranslate() + forward * shift;
		baseCell[0] = FloorDiv(center.x, spacing) - counts[0] / 2;
		baseCell[1] = FloorDiv(camera->GetTranslate().y, spacing) - kCellsBelow_[volumeIndex];
		baseCell[2] = FloorDiv(center.z, spacing) - counts[2] / 2;
	}

	// 少しのずれなら今の格子のまま
	if (hasBaseCell && !needsReset_) {
		bool isNear = true;
		for (int i = 0; i < 3; i++) {
			isNear = isNear && std::abs(baseCell[i] - volume.baseCell[i]) <= kGridSlackCells_;
		}
		if (isNear) {
			for (int i = 0; i < 3; i++) {
				baseCell[i] = volume.baseCell[i];
			}
		}
	}

	for (int i = 0; i < 3; i++) {
		volume.previousBaseCell[i] = hasBaseCell ? volume.baseCell[i] : baseCell[i];
		volume.baseCell[i] = baseCell[i];
	}
	hasBaseCell = true;
	volume.spacing = spacing;
	volume.raysPerProbe = settings_.raysPerProbe;
	volume.normalBias = spacing * 0.1f;
	volume.viewBias = spacing * 0.3f;
	volume.resetAll = needsReset_ ? 1u : 0u;

	// 格子が動いたフレームは全部更新する
	const bool isMoved = volume.baseCell[0] != volume.previousBaseCell[0] || volume.baseCell[1] != volume.previousBaseCell[1]
		|| volume.baseCell[2] != volume.previousBaseCell[2];
	if (needsReset_ || isMoved) {
		volume.probeStride = 1;
		volume.probeOffset = 0;
	} else {
		volume.probeStride = kProbeUpdateStride_;
		volume.probeOffset = updateCounter_[volumeIndex]++ % kProbeUpdateStride_;
	}

	// レイの向きを毎フレームでたらめに回す
	std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
	const float u1 = uniform(random_);
	const float u2 = uniform(random_) * 6.28318530718f;
	const float u3 = uniform(random_) * 6.28318530718f;
	// 一様な回転の四元数 (Shoemake)
	const float qx = std::sqrt(1.0f - u1) * std::sin(u2);
	const float qy = std::sqrt(1.0f - u1) * std::cos(u2);
	const float qz = std::sqrt(u1) * std::sin(u3);
	const float qw = std::sqrt(u1) * std::cos(u3);
	const float rotation[3][3] = {
		{ 1.0f - 2.0f * (qy * qy + qz * qz), 2.0f * (qx * qy - qz * qw), 2.0f * (qx * qz + qy * qw) },
		{ 2.0f * (qx * qy + qz * qw), 1.0f - 2.0f * (qx * qx + qz * qz), 2.0f * (qy * qz - qx * qw) },
		{ 2.0f * (qx * qz - qy * qw), 2.0f * (qy * qz + qx * qw), 1.0f - 2.0f * (qx * qx + qy * qy) },
	};
	for (int r = 0; r < 3; r++) {
		for (int c = 0; c < 3; c++) {
			volume.rayRotation[r][c] = rotation[r][c];
		}
		volume.rayRotation[r][3] = 0.0f;
	}

	uint8_t* frame = volumeMapped_ + kFrameConstantStride_ * frameIndex;
	std::memcpy(frame + kVolumeStride_ * volumeIndex, &volume, sizeof(DDGIVolumeData));
	std::memcpy(frame + kVolumesOffset_ + sizeof(DDGIVolumeData) * volumeIndex, &volume, sizeof(DDGIVolumeData));
}

void DDGIPass::Render(RaytracingScene& scene) {
	if (dxcommon_ == nullptr || lightManager_ == nullptr ||
		(lightManager_->GetData().renderFlags & kRenderFlagGI) == 0 || !scene.IsAvailable() ||
		scene.GetTlasSrvIndex() == RaytracingScene::kInvalidSrvIndex) {
		// 次に点けたときは作り直す
		needsReset_ = true;
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		UpdateVolume(v, frameIndex);
	}

	// レイを飛ばす。跳ね返りは全部の格子を引く
	gpuTimer_.Begin(cList, frameIndex, kTimer_Trace);
	dxcommon_->TransitionResource(rayData_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::DDGITraceCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolumes, GetVolumesGPUVirtualAddress());
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
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIIrradiance, GetIrradianceSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIDistance, GetDistanceSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIProbeData, GetProbeDataSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.uavIndex));
	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, GetVolumeGPUVirtualAddress(v));
		cList->Dispatch((kFixedRays_ + settings_.raysPerProbe) / kRayThreadGroup_, ProbeCount(v) / volumes_[v].probeStride, 1);
	}
	dxcommon_->TransitionResource(rayData_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Trace);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Trace);

	gpuTimer_.Begin(cList, frameIndex, kTimer_Update);

	// 物に埋まったプローブをずらす。反映は次の更新から
	dxcommon_->TransitionResource(probeData_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::DDGIRelocationCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIProbeDataOut, srvManager->GetGPUDescriptorHandle(probeData_.uavIndex));
	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		const uint32_t updatedProbeCount = ProbeCount(v) / volumes_[v].probeStride;
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, GetVolumeGPUVirtualAddress(v));
		cList->Dispatch((updatedProbeCount + kRayThreadGroup_ - 1) / kRayThreadGroup_, 1, 1);
	}
	dxcommon_->TransitionResource(probeData_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	// プローブへ畳み込む
	dxcommon_->TransitionResource(irradiance_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dxcommon_->TransitionResource(distance_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pPipeManager->SetCSPipeline(Pipe::DDGIUpdateIrradianceCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIIrradianceOut, srvManager->GetGPUDescriptorHandle(irradiance_.uavIndex));
	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, GetVolumeGPUVirtualAddress(v));
		cList->Dispatch(ProbeCount(v) / volumes_[v].probeStride, 1, 1);
	}

	pPipeManager->SetCSPipeline(Pipe::DDGIUpdateDistanceCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIDistanceOut, srvManager->GetGPUDescriptorHandle(distance_.uavIndex));
	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, GetVolumeGPUVirtualAddress(v));
		cList->Dispatch(ProbeCount(v) / volumes_[v].probeStride, 1, 1);
	}

	dxcommon_->TransitionResource(irradiance_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	dxcommon_->TransitionResource(distance_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Update);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Update);

	needsReset_ = false;
}

D3D12_GPU_VIRTUAL_ADDRESS DDGIPass::GetVolumeGPUVirtualAddress(uint32_t volumeIndex) const {
	return volumeResource_->GetGPUVirtualAddress() + kFrameConstantStride_ * dxcommon_->GetNowFrameCount()
		+ kVolumeStride_ * volumeIndex;
}

D3D12_GPU_VIRTUAL_ADDRESS DDGIPass::GetVolumesGPUVirtualAddress() const {
	return volumeResource_->GetGPUVirtualAddress() + kFrameConstantStride_ * dxcommon_->GetNowFrameCount() + kVolumesOffset_;
}

D3D12_GPU_DESCRIPTOR_HANDLE DDGIPass::GetIrradianceSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(irradiance_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE DDGIPass::GetDistanceSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(distance_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE DDGIPass::GetProbeDataSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(probeData_.srvIndex);
}

void DDGIPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("GI (probes)")) {
		return;
	}
	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("trace  : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Trace));
	ImGui::Text("update : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Update));
	ImGui::Text("rays : %u", settings_.raysPerProbe);
	for (uint32_t v = 0; v < kVolumeCount_; v++) {
		const DDGIVolumeData& volume = volumes_[v];
		ImGui::Text("%u : %.2fm, %d x %d x %d, base %d %d %d", v, volume.spacing,
			volume.probeCounts[0], volume.probeCounts[1], volume.probeCounts[2], volume.baseCell[0], volume.baseCell[1], volume.baseCell[2]);
	}
	if (ImGui::Button("Reset##gi")) {
		needsReset_ = true;
	}

	static float scale = 0.5f;
	ImGui::SliderFloat("preview scale##gi", &scale, 0.1f, 2.0f);
	ImGui::Image(static_cast<ImTextureID>(GetIrradianceSrvHandle().ptr),
		ImVec2(static_cast<float>(irradiance_.width) * scale, static_cast<float>(irradiance_.height) * scale));
	ImGui::TreePop();
#endif // _DEBUGMODE
}
