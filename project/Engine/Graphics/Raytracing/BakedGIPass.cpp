#include "Engine/Graphics/Raytracing/BakedGIPass.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <json.hpp>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"
#include "Engine/Logger/Logger.h"

using namespace DXC;
using namespace Graphics;

namespace {

	constexpr DXGI_FORMAT kRayDataFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr DXGI_FORMAT kIrradianceFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr DXGI_FORMAT kDistanceFormat = DXGI_FORMAT_R16G16_FLOAT;
	constexpr DXGI_FORMAT kProbeDataFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// DDGICommon.hlsli の kDDGIBakeFlag* と同じ値であること
	constexpr uint32_t kBakeFlagUseOffsets = 1u << 0;

	// 保存するファイル。並びと時刻は header に書く
	constexpr const char* kHeaderFile = "header.json";
	constexpr const char* kSkyFile = "sky.dds";
	constexpr const char* kSunFile = "sun.dds";
	constexpr const char* kDistanceFile = "distance.dds";
	constexpr const char* kProbeDataFile = "probes.dds";
	constexpr int kFileVersion = 1;

	nlohmann::json ToJson(const Math::Vector3& v) {
		return { v.x, v.y, v.z };
	}

	Math::Vector3 ReadVector3(const nlohmann::json& data, const char* key) {
		if (!data.contains(key) || !data[key].is_array() || data[key].size() < 3) {
			return {};
		}
		return { data[key][0].get<float>(), data[key][1].get<float>(), data[key][2].get<float>() };
	}
}


void BakedGIPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;

	constantResource_ = Helper::CreateBufferResource(dxcommon_->GetDevice(), kFrameConstantStride_ * kFrameCount_);
	constantResource_->Map(0, nullptr, reinterpret_cast<void**>(&constantMapped_));
	// 前方描画は焼いていなくても定数を読むので、箱0個の定数を入れておく
	const BakedGIParams empty{};
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		std::memcpy(constantMapped_ + kFrameConstantStride_ * i + kParamsOffset_, &empty, sizeof(empty));
	}
	CreateTextures(Layout{}, 1);
}

void BakedGIPass::Finalize() {
	if (constantResource_) {
		constantMapped_ = nullptr;
		constantResource_.Reset();
	}
	for (AtlasTexture* atlas : { &rayData_, &sky_, &sun_, &distance_, &probeData_ }) {
		atlas->resource.Reset();
	}
	retired_.clear();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

BakedGIPass::Layout BakedGIPass::MakeLayout(const std::vector<VolumeDesc>& volumes) {
	Layout layout;
	float spacingScale = 1.0f;
	for (int attempt = 0; attempt < 32; attempt++) {
		layout = Layout{};
		for (size_t v = 0; v < volumes.size() && v < kMaxVolumes; v++) {
			const VolumeDesc& desc = volumes[v];
			DDGIVolumeData volume;
			volume.spacing = (std::max)(desc.spacing, 0.1f) * spacingScale;
			const float center[3] = { desc.center.x, desc.center.y, desc.center.z };
			const float size[3] = { std::abs(desc.size.x), std::abs(desc.size.y), std::abs(desc.size.z) };
			for (int i = 0; i < 3; i++) {
				// 箱の端から端まで並ぶよう1つ余分に置く
				volume.probeCounts[i] = std::clamp(static_cast<int32_t>(std::ceil(size[i] / volume.spacing)) + 1, 2, kMaxProbesPerAxis_);
				volume.baseCell[i] = static_cast<int32_t>(std::floor((center[i] - size[i] * 0.5f) / volume.spacing));
				volume.previousBaseCell[i] = volume.baseCell[i];
			}
			volume.raysPerProbe = kBakeRays_;
			volume.normalBias = volume.spacing * 0.1f;
			volume.viewBias = volume.spacing * 0.3f;
			volume.probeStride = 1;
			volume.probeOffset = 0;
			volume.firstProbe = layout.probeCount;
			volume.firstTileRow = layout.tileRows;
			layout.tileColumns = (std::max)(layout.tileColumns, static_cast<uint32_t>(volume.probeCounts[0] * volume.probeCounts[1]));
			layout.tileRows += static_cast<uint32_t>(volume.probeCounts[2]);
			layout.probeCount += static_cast<uint32_t>(volume.probeCounts[0] * volume.probeCounts[1] * volume.probeCounts[2]);
			layout.volumes.push_back(volume);
		}
		if (layout.tileColumns <= kMaxTileColumns_ && layout.probeCount <= kMaxProbes_) {
			break;
		}
		// 並べきれなければ間隔を広げてやり直す
		spacingScale *= 1.25f;
	}
	return layout;
}

BakedGIPass::AtlasTexture BakedGIPass::CreateAtlas(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState) {
	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	AtlasTexture atlas;
	atlas.width = (std::max)(width, 1u);
	atlas.height = (std::max)(height, 1u);
	atlas.resource = Helper::CreateTexture2D(device, static_cast<int32_t>(atlas.width), static_cast<int32_t>(atlas.height), format,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, initialState, nullptr);
	atlas.srvIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(atlas.srvIndex, atlas.resource.Get(), format, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	atlas.uavIndex = srvManager->Allocate();
	device->CreateUnorderedAccessView(atlas.resource.Get(), nullptr, &uavDesc, srvManager->GetCPUDescriptorHandle(atlas.uavIndex));
	return atlas;
}

void BakedGIPass::RetireAtlas(AtlasTexture& atlas) {
	if (!atlas.resource) {
		return;
	}
	// 前のフレームの描画がまだ読んでいるかもしれない
	retired_.push_back({ atlas.resource, kFrameCount_ + 1 });
	SRVManager::GetInstance()->FreeDeferred(atlas.srvIndex);
	SRVManager::GetInstance()->FreeDeferred(atlas.uavIndex);
	atlas = AtlasTexture{};
}

void BakedGIPass::CreateTextures(const Layout& layout, uint32_t sunKeyCount) {
	for (AtlasTexture* atlas : { &rayData_, &sky_, &sun_, &distance_, &probeData_ }) {
		RetireAtlas(*atlas);
	}
	const uint32_t irradianceTile = kIrradianceTexels_ + 2;
	const uint32_t distanceTile = kDistanceTexels_ + 2;
	rayData_ = CreateAtlas(kFixedRays_ + kBakeRays_, layout.probeCount, kRayDataFormat, kReadState);
	// 空の上・下の段と、時刻ごとの太陽の段を縦へ積む
	sky_ = CreateAtlas(layout.tileColumns * irradianceTile, 2 * layout.tileRows * irradianceTile, kIrradianceFormat, kReadState);
	sun_ = CreateAtlas(layout.tileColumns * irradianceTile, (std::max)(sunKeyCount, 1u) * layout.tileRows * irradianceTile,
		kIrradianceFormat, kReadState);
	distance_ = CreateAtlas(layout.tileColumns * distanceTile, layout.tileRows * distanceTile, kDistanceFormat, kReadState);
	probeData_ = CreateAtlas(layout.tileColumns, layout.tileRows, kProbeDataFormat, kReadState);

	layout_ = layout;
	for (DDGIVolumeData& volume : layout_.volumes) {
		volume.distanceAtlasSize[0] = static_cast<float>(distance_.width);
		volume.distanceAtlasSize[1] = static_cast<float>(distance_.height);
		volume.irradianceAtlasSize[0] = static_cast<float>(sky_.width);
		volume.irradianceAtlasSize[1] = static_cast<float>(sky_.height);
	}
}

void BakedGIPass::StartBake(const std::vector<VolumeDesc>& volumes, const std::vector<SunKey>& sunKeys, bool hasSunHours,
	const std::string& directory) {
	if (dxcommon_ == nullptr || volumes.empty()) {
		return;
	}
	bakedVolumes_.assign(volumes.begin(), volumes.begin() + (std::min)(volumes.size(), static_cast<size_t>(kMaxVolumes)));
	sunKeys_ = sunKeys;
	if (sunKeys_.empty()) {
		sunKeys_.push_back(SunKey{});
	}
	hasSunHours_ = hasSunHours;
	CreateTextures(MakeLayout(bakedVolumes_), static_cast<uint32_t>(sunKeys_.size()));

	jobs_.clear();
	jobs_.push_back({ kBakeSkyUp, 0 });
	jobs_.push_back({ kBakeSkyDown, 0 });
	for (uint32_t k = 0; k < sunKeys_.size(); k++) {
		jobs_.push_back({ kBakeSun, k });
	}
	jobIndex_ = 0;
	iteration_ = 0;
	doneIterations_ = 0;
	totalIterations_ = kRelocationIterations_ + static_cast<uint32_t>(jobs_.size()) * kBakeIterations_;
	bakeDirectory_ = directory;
	saveCountdown_ = 0;
	hasData_ = false;
	isBaking_ = true;
}

void BakedGIPass::CancelBake() {
	isBaking_ = false;
	hasData_ = false;
}

void BakedGIPass::Clear() {
	isBaking_ = false;
	hasData_ = false;
	bakedVolumes_.clear();
	sunKeys_.clear();
}

float BakedGIPass::GetBakeProgress() const {
	return totalIterations_ > 0 ? static_cast<float>(doneIterations_) / static_cast<float>(totalIterations_) : 0.0f;
}

void BakedGIPass::Render(RaytracingScene& scene) {
	if (dxcommon_ == nullptr) {
		return;
	}
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	std::erase_if(retired_, [](Retired& r) { return r.framesLeft-- == 0; });

	// GPU が焼き終えてから読み戻す
	if (saveCountdown_ > 0 && --saveCountdown_ == 0) {
		Save(bakeDirectory_);
	}

	if (isBaking_ && scene.IsAvailable() && scene.GetTlasSrvIndex() != RaytracingScene::kInvalidSrvIndex &&
		scene.GetInstanceDataSrvIndex() != RaytracingScene::kInvalidSrvIndex) {
		for (uint32_t slot = 0; slot < kIterationsPerFrame_ && isBaking_; slot++) {
			RunIteration(scene, slot);
			doneIterations_++;
			const uint32_t jobIterations = kBakeIterations_ + (jobIndex_ == 0 ? kRelocationIterations_ : 0);
			if (++iteration_ >= jobIterations) {
				iteration_ = 0;
				if (++jobIndex_ >= jobs_.size()) {
					isBaking_ = false;
					hasData_ = true;
					saveCountdown_ = kFrameCount_ + 1;
				}
			}
		}
	}

	UpdateParams(frameIndex);
}

void BakedGIPass::RunIteration(RaytracingScene& scene, uint32_t slot) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	const Job& job = jobs_[jobIndex_];
	const bool isFirstJob = jobIndex_ == 0;
	const uint32_t i = iteration_;
	// 最初の仕事は、埋まったプローブをずらしてから平均を取り直す
	const bool isRelocating = isFirstJob && i < kRelocationIterations_;
	const uint32_t averaged = isFirstJob ? (i >= kRelocationIterations_ ? i - kRelocationIterations_ : 0) : i;
	const bool isReset = (i == 0) || (isFirstJob && i == kRelocationIterations_);

	AtlasTexture& target = (job.component == kBakeSun) ? sun_ : sky_;
	uint32_t rowOffset = 0;
	if (job.component == kBakeSkyDown) {
		rowOffset = layout_.tileRows;
	} else if (job.component == kBakeSun) {
		rowOffset = job.sunKey * layout_.tileRows;
	}

	// レイの向きを毎回でたらめに回す (DDGIPass と同じ)
	std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
	const float u1 = uniform(random_);
	const float u2 = uniform(random_) * 6.28318530718f;
	const float u3 = uniform(random_) * 6.28318530718f;
	const float qx = std::sqrt(1.0f - u1) * std::sin(u2);
	const float qy = std::sqrt(1.0f - u1) * std::cos(u2);
	const float qz = std::sqrt(u1) * std::sin(u3);
	const float qw = std::sqrt(u1) * std::cos(u3);
	const float rotation[3][3] = {
		{ 1.0f - 2.0f * (qy * qy + qz * qz), 2.0f * (qx * qy - qz * qw), 2.0f * (qx * qz + qy * qw) },
		{ 2.0f * (qx * qy + qz * qw), 1.0f - 2.0f * (qx * qx + qz * qz), 2.0f * (qy * qz - qx * qw) },
		{ 2.0f * (qx * qz - qy * qw), 2.0f * (qy * qz + qx * qw), 1.0f - 2.0f * (qx * qx + qy * qy) },
	};

	const Math::Vector3& toSun = sunKeys_[(std::min)(static_cast<size_t>(job.sunKey), sunKeys_.size() - 1)].toSun;
	const uint64_t base = kFrameConstantStride_ * frameIndex + kIterationStride_ * slot;
	DDGIVolumeData all[kMaxVolumes] = {};
	for (size_t v = 0; v < layout_.volumes.size(); v++) {
		DDGIVolumeData volume = layout_.volumes[v];
		volume.hysteresis = isRelocating ? 0.0f : static_cast<float>(averaged) / static_cast<float>(averaged + 1);
		volume.resetAll = isReset ? 1u : 0u;
		volume.bakeComponent = job.component;
		volume.irradianceRowOffset = rowOffset;
		volume.irradianceAtlasSize[0] = static_cast<float>(target.width);
		volume.irradianceAtlasSize[1] = static_cast<float>(target.height);
		volume.bakeFlags = (isFirstJob && i == 0) ? 0u : kBakeFlagUseOffsets;
		volume.bakeToSun[0] = toSun.x;
		volume.bakeToSun[1] = toSun.y;
		volume.bakeToSun[2] = toSun.z;
		for (int r = 0; r < 3; r++) {
			for (int c = 0; c < 3; c++) {
				volume.rayRotation[r][c] = rotation[r][c];
			}
			volume.rayRotation[r][3] = 0.0f;
		}
		std::memcpy(constantMapped_ + base + kVolumeStride_ * v, &volume, sizeof(volume));
		all[v] = volume;
	}
	// 使わない箱はプローブ0個
	std::memcpy(constantMapped_ + base + kVolumesOffset_, all, sizeof(all));
	const D3D12_GPU_VIRTUAL_ADDRESS address = constantResource_->GetGPUVirtualAddress() + base;
	auto volumeAddress = [&](size_t v) { return address + kVolumeStride_ * v; };

	// 跳ね返りは焼きかけの同じ成分を引く
	dxcommon_->TransitionResource(rayData_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::BakedGITraceCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolumes, address + kVolumesOffset_);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene.GetTlasSrvIndex()));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayInstances, srvManager->GetGPUDescriptorHandle(scene.GetInstanceDataSrvIndex()));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayVertices, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayIndices, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kTextures, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIIrradiance, srvManager->GetGPUDescriptorHandle(target.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIDistance, srvManager->GetGPUDescriptorHandle(distance_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIProbeData, srvManager->GetGPUDescriptorHandle(probeData_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.uavIndex));
	for (size_t v = 0; v < layout_.volumes.size(); v++) {
		const DDGIVolumeData& volume = layout_.volumes[v];
		const uint32_t probeCount = static_cast<uint32_t>(volume.probeCounts[0] * volume.probeCounts[1] * volume.probeCounts[2]);
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, volumeAddress(v));
		cList->Dispatch((kFixedRays_ + kBakeRays_) / kRayThreadGroup_, probeCount, 1);
	}
	dxcommon_->TransitionResource(rayData_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	if (isRelocating) {
		dxcommon_->TransitionResource(probeData_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pPipeManager->SetCSPipeline(Pipe::DDGIRelocationCS);
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.srvIndex));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIProbeDataOut, srvManager->GetGPUDescriptorHandle(probeData_.uavIndex));
		for (size_t v = 0; v < layout_.volumes.size(); v++) {
			const DDGIVolumeData& volume = layout_.volumes[v];
			const uint32_t probeCount = static_cast<uint32_t>(volume.probeCounts[0] * volume.probeCounts[1] * volume.probeCounts[2]);
			pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, volumeAddress(v));
			cList->Dispatch((probeCount + kRayThreadGroup_ - 1) / kRayThreadGroup_, 1, 1);
		}
		dxcommon_->TransitionResource(probeData_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	}

	dxcommon_->TransitionResource(target.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::DDGIUpdateIrradianceCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIIrradianceOut, srvManager->GetGPUDescriptorHandle(target.uavIndex));
	for (size_t v = 0; v < layout_.volumes.size(); v++) {
		const DDGIVolumeData& volume = layout_.volumes[v];
		pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, volumeAddress(v));
		cList->Dispatch(static_cast<UINT>(volume.probeCounts[0] * volume.probeCounts[1] * volume.probeCounts[2]), 1, 1);
	}
	dxcommon_->TransitionResource(target.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	// 距離は形だけで決まるので最初の仕事で焼けば足りる
	if (isFirstJob) {
		dxcommon_->TransitionResource(distance_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pPipeManager->SetCSPipeline(Pipe::DDGIUpdateDistanceCS);
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIRayData, srvManager->GetGPUDescriptorHandle(rayData_.srvIndex));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDDGIDistanceOut, srvManager->GetGPUDescriptorHandle(distance_.uavIndex));
		for (size_t v = 0; v < layout_.volumes.size(); v++) {
			const DDGIVolumeData& volume = layout_.volumes[v];
			pPipeManager->SetComputeRootCBV(cList, RootName::kDDGIVolume, volumeAddress(v));
			cList->Dispatch(static_cast<UINT>(volume.probeCounts[0] * volume.probeCounts[1] * volume.probeCounts[2]), 1, 1);
		}
		dxcommon_->TransitionResource(distance_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	}
}

void BakedGIPass::UpdateParams(uint32_t frameIndex) {
	BakedGIParams params{};
	if (hasData_ && !isBaking_) {
		params.volumeCount = static_cast<uint32_t>(layout_.volumes.size());
		for (size_t v = 0; v < layout_.volumes.size(); v++) {
			params.volumes[v] = layout_.volumes[v];
		}
		params.skyDownRow = layout_.tileRows;
		params.sunAtlasSize[0] = static_cast<float>(sun_.width);
		params.sunAtlasSize[1] = static_cast<float>(sun_.height);

		// 太陽の色と強さは0番の平行光源 (夜は月)
		float radiance[3] = {};
		if (lightManager_ && lightManager_->GetNumDirectionalLights() > 0) {
			const DirectionalLight& sun = lightManager_->GetData().directionalLights[0];
			radiance[0] = sun.color.x * sun.intensity;
			radiance[1] = sun.color.y * sun.intensity;
			radiance[2] = sun.color.z * sun.intensity;
		}

		uint32_t keyA = 0;
		uint32_t keyB = 0;
		float blend = 0.0f;
		if (hasSunHours_ && sunKeys_.size() > 1) {
			if (hasSunHour_) {
				// 夜は12時間ずれた太陽を月の色で引く (Level::ApplyTimeOfDay と同じ)
				float hour = sunHour_;
				if (hour < sunKeys_.front().hour || hour > sunKeys_.back().hour) {
					hour = std::fmod(hour + 12.0f, 24.0f);
				}
				if (hour < sunKeys_.front().hour || hour > sunKeys_.back().hour) {
					radiance[0] = radiance[1] = radiance[2] = 0.0f;
				} else {
					for (uint32_t k = 0; k + 1 < sunKeys_.size(); k++) {
						if (hour <= sunKeys_[k + 1].hour) {
							keyA = k;
							keyB = k + 1;
							blend = (hour - sunKeys_[k].hour) / (std::max)(sunKeys_[k + 1].hour - sunKeys_[k].hour, 1e-4f);
							break;
						}
					}
				}
			} else if (lightManager_ && lightManager_->GetNumDirectionalLights() > 0) {
				// 時刻で変えないなら、平行光源の向きにいちばん近い時刻を使う
				const Math::Vector3 toLight = lightManager_->GetData().directionalLights[0].direction * -1.0f;
				float best = -2.0f;
				for (uint32_t k = 0; k < sunKeys_.size(); k++) {
					const float d = Math::Vector3::Dot(toLight, sunKeys_[k].toSun);
					if (d > best) {
						best = d;
						keyA = keyB = k;
					}
				}
			}
		}
		params.sunRowA = keyA * layout_.tileRows;
		params.sunRowB = keyB * layout_.tileRows;
		params.sunBlend = std::clamp(blend, 0.0f, 1.0f);
		params.sunRadiance[0] = radiance[0];
		params.sunRadiance[1] = radiance[1];
		params.sunRadiance[2] = radiance[2];
	}
	std::memcpy(constantMapped_ + kFrameConstantStride_ * frameIndex + kParamsOffset_, &params, sizeof(params));
}

bool BakedGIPass::Save(const std::string& directory) {
	if (dxcommon_ == nullptr || !hasData_) {
		return false;
	}
	std::error_code error;
	std::filesystem::create_directories(directory, error);
	ID3D12CommandQueue* queue = dxcommon_->GetDXCommand()->GetQueue();
	const std::pair<const AtlasTexture*, const char*> files[] = {
		{ &sky_, kSkyFile }, { &sun_, kSunFile }, { &distance_, kDistanceFile }, { &probeData_, kProbeDataFile },
	};
	for (const auto& [atlas, name] : files) {
		DirectX::ScratchImage image;
		HRESULT hr = DirectX::CaptureTexture(queue, atlas->resource.Get(), false, image, kReadState, kReadState);
		if (SUCCEEDED(hr)) {
			const std::wstring path = (std::filesystem::path(directory) / name).wstring();
			hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::DDS_FLAGS_NONE, path.c_str());
		}
		if (FAILED(hr)) {
			Logger::Log(std::string("Baked GI save failed: ") + name);
			return false;
		}
	}

	nlohmann::json header;
	header["version"] = kFileVersion;
	nlohmann::json volumes = nlohmann::json::array();
	for (const VolumeDesc& desc : bakedVolumes_) {
		volumes.push_back({ { "center", ToJson(desc.center) }, { "size", ToJson(desc.size) }, { "spacing", desc.spacing } });
	}
	header["volumes"] = std::move(volumes);
	nlohmann::json keys = nlohmann::json::array();
	for (const SunKey& key : sunKeys_) {
		keys.push_back({ { "hour", key.hour }, { "toSun", ToJson(key.toSun) } });
	}
	header["sunKeys"] = std::move(keys);
	header["hasSunHours"] = hasSunHours_;
	Core::JsonSerializer::SerializeJsonData(header, (std::filesystem::path(directory) / kHeaderFile).string());
	Logger::Log("Baked GI saved: " + directory);
	return true;
}

bool BakedGIPass::Load(const std::string& directory) {
	hasData_ = false;
	isBaking_ = false;
	saveCountdown_ = 0;
	if (dxcommon_ == nullptr) {
		return false;
	}
	const std::filesystem::path headerPath = std::filesystem::path(directory) / kHeaderFile;
	if (!std::filesystem::exists(headerPath)) {
		bakedVolumes_.clear();
		return false;
	}
	const nlohmann::json header = Core::JsonSerializer::DeserializeJsonData(headerPath.string());
	if (!header.is_object() || header.value("version", 0) != kFileVersion) {
		return false;
	}
	std::vector<VolumeDesc> volumes;
	if (header.contains("volumes")) {
		for (const auto& data : header["volumes"]) {
			VolumeDesc desc;
			desc.center = ReadVector3(data, "center");
			desc.size = ReadVector3(data, "size");
			desc.spacing = data.value("spacing", desc.spacing);
			volumes.push_back(desc);
		}
	}
	std::vector<SunKey> keys;
	if (header.contains("sunKeys")) {
		for (const auto& data : header["sunKeys"]) {
			SunKey key;
			key.hour = data.value("hour", key.hour);
			key.toSun = ReadVector3(data, "toSun");
			keys.push_back(key);
		}
	}
	if (volumes.empty() || keys.empty()) {
		return false;
	}
	bakedVolumes_ = volumes;
	sunKeys_ = keys;
	hasSunHours_ = header.value("hasSunHours", false);
	CreateTextures(MakeLayout(bakedVolumes_), static_cast<uint32_t>(sunKeys_.size()));

	// 大きさか形式が違えば焼き直しが要る
	ID3D12GraphicsCommandList* list = dxcommon_->GetImmediateList();
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediates;
	const std::pair<AtlasTexture*, const char*> files[] = {
		{ &sky_, kSkyFile }, { &sun_, kSunFile }, { &distance_, kDistanceFile }, { &probeData_, kProbeDataFile },
	};
	for (const auto& [atlas, name] : files) {
		DirectX::ScratchImage image;
		const std::wstring path = (std::filesystem::path(directory) / name).wstring();
		if (FAILED(DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image))) {
			Logger::Log(std::string("Baked GI load failed: ") + name);
			return false;
		}
		const DirectX::TexMetadata& meta = image.GetMetadata();
		const D3D12_RESOURCE_DESC desc = atlas->resource->GetDesc();
		if (meta.width != desc.Width || meta.height != desc.Height || meta.format != desc.Format) {
			Logger::Log(std::string("Baked GI size mismatch: ") + name);
			return false;
		}
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		DirectX::PrepareUpload(dxcommon_->GetDevice(), image.GetImages(), image.GetImageCount(), meta, subresources);
		const uint64_t size = GetRequiredIntermediateSize(atlas->resource.Get(), 0, static_cast<UINT>(subresources.size()));
		intermediates.push_back(Helper::CreateBufferResource(dxcommon_->GetDevice(), static_cast<size_t>(size)));

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = atlas->resource.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = kReadState;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		list->ResourceBarrier(1, &barrier);
		UpdateSubresources(list, atlas->resource.Get(), intermediates.back().Get(), 0, 0,
			static_cast<UINT>(subresources.size()), subresources.data());
		std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
		list->ResourceBarrier(1, &barrier);
	}
	// 写し終わるまで待つ
	dxcommon_->CommandExecution();
	hasData_ = true;
	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS BakedGIPass::GetParamsGPUVirtualAddress() const {
	return constantResource_->GetGPUVirtualAddress() + kFrameConstantStride_ * dxcommon_->GetNowFrameCount() + kParamsOffset_;
}

D3D12_GPU_DESCRIPTOR_HANDLE BakedGIPass::GetSkySrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(sky_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE BakedGIPass::GetSunSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(sun_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE BakedGIPass::GetDistanceSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(distance_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE BakedGIPass::GetProbeDataSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(probeData_.srvIndex);
}

void BakedGIPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Baked GI")) {
		return;
	}
	if (isBaking_) {
		ImGui::ProgressBar(GetBakeProgress());
	} else {
		ImGui::Text(hasData_ ? "baked" : "none");
	}
	for (size_t v = 0; v < layout_.volumes.size(); v++) {
		const DDGIVolumeData& volume = layout_.volumes[v];
		ImGui::Text("%zu : %.2fm, %d x %d x %d", v, volume.spacing, volume.probeCounts[0], volume.probeCounts[1], volume.probeCounts[2]);
	}
	ImGui::Text("sun keys : %zu", sunKeys_.size());
	static float scale = 0.5f;
	ImGui::SliderFloat("preview scale##baked", &scale, 0.1f, 2.0f);
	ImGui::Image(static_cast<ImTextureID>(GetSkySrvHandle().ptr),
		ImVec2(static_cast<float>(sky_.width) * scale, static_cast<float>(sky_.height) * scale));
	ImGui::TreePop();
#endif // _DEBUGMODE
}
