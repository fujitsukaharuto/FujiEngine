#include "Engine/Graphics/Raytracing/ReflectionProbePass.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <numeric>

#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Light/ClusteredLightPass.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/IBL/IBLBaker.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace DXC;
using namespace Graphics;

namespace {

	// HDR。a は物に当たった割合
	constexpr DXGI_FORMAT kProbeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// ReflectionProbe.hlsli の kReflectionProbe* と同じ値であること
	constexpr uint32_t kFlagBoxProjection = 1u << 0;
	constexpr uint32_t kSliceShift = 8;

	uint32_t DispatchCount(uint32_t pixels, uint32_t group) {
		return (pixels + group - 1) / group;
	}

	void UAVBarrier(ID3D12GraphicsCommandList* list, ID3D12Resource* resource) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = resource;
		list->ResourceBarrier(1, &barrier);
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateCubeArray(ID3D12Device* device, uint32_t size, uint32_t cubes, uint32_t mips) {
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = size;
		desc.Height = size;
		desc.DepthOrArraySize = static_cast<UINT16>(6 * cubes);
		desc.MipLevels = static_cast<UINT16>(mips);
		desc.Format = kProbeFormat;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		[[maybe_unused]] const HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, kReadState, nullptr,
			IID_PPV_ARGS(&resource));
		assert(SUCCEEDED(hr));
		return resource;
	}
}


void ReflectionProbePass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	CreateResources();

	constantResource_ = Helper::CreateBufferResource(dxcommon_->GetDevice(), kFrameConstantStride_ * kFrameCount_);
	constantResource_->Map(0, nullptr, reinterpret_cast<void**>(&constantMapped_));
	// 前方描画は焼いていなくても定数を読むので、箱0個の定数を入れておく
	const ProbeParams empty{};
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		std::memcpy(constantMapped_ + kFrameConstantStride_ * i + kParamsOffset_, &empty, sizeof(empty));
	}
}

void ReflectionProbePass::CreateResources() {
	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	capture_ = CreateCubeArray(device, kCaptureSize_, 1, kCaptureMipLevels_);
	probeArray_ = CreateCubeArray(device, kCaptureSize_, kMaxProbes, kPrefilteredMipLevels_);

	// 畳むときはキューブとして読む
	captureSrvIndex_ = srvManager->Allocate();
	srvManager->CreateTextureSRV(captureSrvIndex_, capture_.Get(), kProbeFormat, kCaptureMipLevels_, true);

	D3D12_SHADER_RESOURCE_VIEW_DESC arraySrv{};
	arraySrv.Format = kProbeFormat;
	arraySrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	arraySrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	arraySrv.TextureCubeArray.MostDetailedMip = 0;
	arraySrv.TextureCubeArray.MipLevels = kPrefilteredMipLevels_;
	arraySrv.TextureCubeArray.First2DArrayFace = 0;
	arraySrv.TextureCubeArray.NumCubes = kMaxProbes;
	probesSrvIndex_ = srvManager->Allocate();
	device->CreateShaderResourceView(probeArray_.Get(), &arraySrv, srvManager->GetCPUDescriptorHandle(probesSrvIndex_));

	// 書く側は配列として張る。ミップごとに1つ
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
	uav.Format = kProbeFormat;
	uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	uav.Texture2DArray.FirstArraySlice = 0;
	uav.Texture2DArray.ArraySize = kCubeFaceCount_;
	for (uint32_t mip = 0; mip < kCaptureMipLevels_; mip++) {
		uav.Texture2DArray.MipSlice = mip;
		captureUavIndex_[mip] = srvManager->Allocate();
		device->CreateUnorderedAccessView(capture_.Get(), nullptr, &uav, srvManager->GetCPUDescriptorHandle(captureUavIndex_[mip]));
	}
	uav.Texture2DArray.ArraySize = kCubeFaceCount_ * kMaxProbes;
	for (uint32_t mip = 0; mip < kPrefilteredMipLevels_; mip++) {
		uav.Texture2DArray.MipSlice = mip;
		probesUavIndex_[mip] = srvManager->Allocate();
		device->CreateUnorderedAccessView(probeArray_.Get(), nullptr, &uav, srvManager->GetCPUDescriptorHandle(probesUavIndex_[mip]));
	}
}

void ReflectionProbePass::Finalize() {
	if (constantResource_) {
		constantMapped_ = nullptr;
		constantResource_.Reset();
	}
	capture_.Reset();
	probeArray_.Reset();
	probes_.clear();
	bakeQueue_.clear();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

bool ReflectionProbePass::IsSameProbe(const ProbeDesc& a, const ProbeDesc& b) {
	return (a.center - b.center).Length() < 1e-3f && (a.size - b.size).Length() < 1e-3f &&
		a.boxProjection == b.boxProjection;
}

void ReflectionProbePass::SetProbes(const std::vector<ProbeDesc>& probes) {
	const size_t count = (std::min)(probes.size(), static_cast<size_t>(kMaxProbes));
	for (uint32_t i = 0; i < count; i++) {
		// 溶かす幅と明るさは変わっても焼き直さない
		const bool isChanged = i >= probes_.size() || !IsSameProbe(probes[i], probes_[i]);
		if (isChanged && std::find(bakeQueue_.begin(), bakeQueue_.end(), i) == bakeQueue_.end()) {
			bakeQueue_.push_back(i);
		}
		if (isChanged) {
			isBaked_[i] = false;
		}
	}
	for (size_t i = count; i < kMaxProbes; i++) {
		isBaked_[i] = false;
	}
	probes_.assign(probes.begin(), probes.begin() + count);
	std::erase_if(bakeQueue_, [count](uint32_t slot) { return slot >= count; });
}

void ReflectionProbePass::RequestRebake() {
	for (uint32_t i = 0; i < probes_.size(); i++) {
		if (std::find(bakeQueue_.begin(), bakeQueue_.end(), i) == bakeQueue_.end()) {
			bakeQueue_.push_back(i);
		}
	}
}

uint32_t ReflectionProbePass::GetBakedCount() const {
	uint32_t count = 0;
	for (uint32_t i = 0; i < probes_.size(); i++) {
		count += isBaked_[i] ? 1u : 0u;
	}
	return count;
}

ReflectionProbePass::LightingKey ReflectionProbePass::MakeLightingKey(uint32_t clusteredLightCount) const {
	LightingKey key;
	key.clusteredLightCount = clusteredLightCount;
	if (lightManager_ == nullptr) {
		return key;
	}
	const AllLightsData& lights = lightManager_->GetData();
	float* v = key.values;
	if (lights.numDirectionalLights > 0) {
		const DirectionalLight& sun = lights.directionalLights[0];
		*v++ = sun.direction.x;
		*v++ = sun.direction.y;
		*v++ = sun.direction.z;
		*v++ = sun.color.x * sun.intensity;
		*v++ = sun.color.y * sun.intensity;
		*v++ = sun.color.z * sun.intensity;
	} else {
		v += 6;
	}
	*v++ = lights.ambientSkyColor.x * lights.ambientIntensity;
	*v++ = lights.ambientSkyColor.y * lights.ambientIntensity;
	*v++ = lights.ambientSkyColor.z * lights.ambientIntensity;
	*v++ = lights.ambientGroundColor.x * lights.ambientIntensity;
	*v++ = lights.ambientGroundColor.y * lights.ambientIntensity;
	*v++ = lights.ambientGroundColor.z * lights.ambientIntensity;
	*v++ = lights.environmentTint.x;
	*v++ = lights.environmentTint.y;
	*v++ = lights.environmentTint.z;
	*v++ = lights.fog.density;
	*v++ = lights.fog.secondDensity;
	key.punctualCount = lights.numPointLights + lights.numSpotLights;
	return key;
}

bool ReflectionProbePass::IsSameLighting(const LightingKey& a, const LightingKey& b) {
	if (a.clusteredLightCount != b.clusteredLightCount || a.punctualCount != b.punctualCount) {
		return false;
	}
	for (size_t i = 0; i < std::size(a.values); i++) {
		// ゆっくりした変化はある程度たまってから焼き直す
		if (std::abs(a.values[i] - b.values[i]) > 0.01f * (std::max)(std::abs(b.values[i]), 0.05f)) {
			return false;
		}
	}
	return true;
}

void ReflectionProbePass::Render(RaytracingScene& scene, const ClusteredLightPass& clusteredLights, bool hasRayLooks) {
	if (dxcommon_ == nullptr) {
		return;
	}
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	const bool canTrace = scene.IsAvailable() && scene.GetTlasSrvIndex() != RaytracingScene::kInvalidSrvIndex &&
		scene.GetInstanceDataSrvIndex() != RaytracingScene::kInvalidSrvIndex;

	// 物の見た目を集めていないフレームには焼かない
	if (canTrace && hasRayLooks && !bakeQueue_.empty()) {
		const uint32_t slot = bakeQueue_.front();
		bakeQueue_.erase(bakeQueue_.begin());
		BakeProbe(scene, clusteredLights, slot, frameIndex);
		bakedLighting_ = MakeLightingKey(clusteredLights.GetLightCount());
		hasLightingKey_ = true;
		framesSinceRebake_ = 0;
	}
	framesSinceRebake_++;

	// 光が変わったら次のフレームから焼き直す
	if (isAutoRebake_ && hasLightingKey_ && bakeQueue_.empty() && !probes_.empty() &&
		framesSinceRebake_ >= kAutoRebakeInterval_ &&
		!IsSameLighting(MakeLightingKey(clusteredLights.GetLightCount()), bakedLighting_)) {
		RequestRebake();
	}

	UpdateParams(frameIndex);
}

void ReflectionProbePass::BakeProbe(RaytracingScene& scene, const ClusteredLightPass& clusteredLights, uint32_t slot, uint32_t frameIndex) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const ProbeDesc& desc = probes_[slot];
	const uint64_t frameBase = kFrameConstantStride_ * frameIndex;
	const D3D12_GPU_VIRTUAL_ADDRESS constantBase = constantResource_->GetGPUVirtualAddress() + frameBase;

	CaptureParam capture{};
	capture.capturePosition[0] = desc.center.x;
	capture.capturePosition[1] = desc.center.y;
	capture.capturePosition[2] = desc.center.z;
	capture.clusteredLightCount = clusteredLights.GetLightCount();
	capture.maxDistance = kCaptureMaxDistance_;
	capture.captureSize = kCaptureSize_;
	capture.textureLod = kCaptureTextureLod_;
	std::memcpy(constantMapped_ + frameBase + kCaptureOffset_, &capture, sizeof(capture));

	// 周りを焼く。光らせ方は映り込みのレイと同じ
	dxcommon_->TransitionResource(capture_.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ReflectionProbeCaptureCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kReflectionProbeCaptureParam, constantBase + kCaptureOffset_);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene.GetTlasSrvIndex()));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayInstances, srvManager->GetGPUDescriptorHandle(scene.GetInstanceDataSrvIndex()));
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
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusteredLights, clusteredLights.GetLightsSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeCaptureOut, srvManager->GetGPUDescriptorHandle(captureUavIndex_[0]));
	const uint32_t groups = DispatchCount(kCaptureSize_, kThreadGroupSize_);
	cList->Dispatch(groups, groups, kCubeFaceCount_);

	// 粗いミップを作る。1段ずつ前の段を読む
	pPipeManager->SetCSPipeline(Pipe::ReflectionProbeDownsampleCS);
	for (uint32_t mip = 1; mip < kCaptureMipLevels_; mip++) {
		UAVBarrier(cList, capture_.Get());
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDownsampleSource, srvManager->GetGPUDescriptorHandle(captureUavIndex_[mip - 1]));
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kDownsampleOut, srvManager->GetGPUDescriptorHandle(captureUavIndex_[mip]));
		const uint32_t mipGroups = DispatchCount((std::max)(kCaptureSize_ >> mip, 1u), kThreadGroupSize_);
		cList->Dispatch(mipGroups, mipGroups, kCubeFaceCount_);
	}
	dxcommon_->TransitionResource(capture_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	// 粗さごとに畳んで配列のこの箱の場所へ書く
	dxcommon_->TransitionResource(probeArray_.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ReflectionProbePrefilterCS);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbeCapture, srvManager->GetGPUDescriptorHandle(captureSrvIndex_));
	for (uint32_t mip = 0; mip < kPrefilteredMipLevels_; mip++) {
		PrefilterParam param{};
		param.roughness = static_cast<float>(mip) / static_cast<float>(kPrefilteredMipLevels_ - 1);
		// 粗さ0は散らす先が1点しかない
		param.sampleCount = (mip == 0) ? 1 : kPrefilterSampleCount_;
		param.sourceSize = kCaptureSize_;
		param.sliceOffset = slot * kCubeFaceCount_;
		param.outputSize = kCaptureSize_ >> mip;
		const uint64_t offset = kPrefilterOffset_ + 256 * mip;
		std::memcpy(constantMapped_ + frameBase + offset, &param, sizeof(param));

		pPipeManager->SetComputeRootCBV(cList, RootName::kProbePrefilterParam, constantBase + offset);
		pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kProbePrefilteredOut, srvManager->GetGPUDescriptorHandle(probesUavIndex_[mip]));
		const uint32_t mipGroups = DispatchCount(param.outputSize, kThreadGroupSize_);
		cList->Dispatch(mipGroups, mipGroups, kCubeFaceCount_);
	}
	dxcommon_->TransitionResource(probeArray_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	isBaked_[slot] = true;
	bakeCount_++;
}

void ReflectionProbePass::UpdateParams(uint32_t frameIndex) {
	ProbeParams params{};
	// 大きい箱から並べる。小さな箱が勝つ
	std::vector<uint32_t> order(probes_.size());
	std::iota(order.begin(), order.end(), 0u);
	auto volume = [this](uint32_t i) { return std::abs(probes_[i].size.x * probes_[i].size.y * probes_[i].size.z); };
	std::stable_sort(order.begin(), order.end(), [&volume](uint32_t a, uint32_t b) { return volume(a) > volume(b); });
	for (uint32_t slot : order) {
		if (!isBaked_[slot]) {
			continue;
		}
		const ProbeDesc& desc = probes_[slot];
		const Math::Vector3 half = { std::abs(desc.size.x) * 0.5f, std::abs(desc.size.y) * 0.5f, std::abs(desc.size.z) * 0.5f };
		ProbeData& data = params.probes[params.probeCount++];
		data.capturePosition[0] = desc.center.x;
		data.capturePosition[1] = desc.center.y;
		data.capturePosition[2] = desc.center.z;
		data.blendDistance = (std::max)(desc.blendDistance, 0.01f);
		data.boxMin[0] = desc.center.x - half.x;
		data.boxMin[1] = desc.center.y - half.y;
		data.boxMin[2] = desc.center.z - half.z;
		data.boxMax[0] = desc.center.x + half.x;
		data.boxMax[1] = desc.center.y + half.y;
		data.boxMax[2] = desc.center.z + half.z;
		data.intensity = (std::max)(desc.intensity, 0.0f);
		data.flags = (desc.boxProjection ? kFlagBoxProjection : 0u) | (slot << kSliceShift);
	}
	activeProbeCount_ = params.probeCount;
	std::memcpy(constantMapped_ + kFrameConstantStride_ * frameIndex + kParamsOffset_, &params, sizeof(params));
}

D3D12_GPU_VIRTUAL_ADDRESS ReflectionProbePass::GetParamsGPUVirtualAddress() const {
	return constantResource_->GetGPUVirtualAddress() + kFrameConstantStride_ * dxcommon_->GetNowFrameCount() + kParamsOffset_;
}

D3D12_GPU_DESCRIPTOR_HANDLE ReflectionProbePass::GetProbesSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(probesSrvIndex_);
}

void ReflectionProbePass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Reflection probes")) {
		return;
	}
	ImGui::Text("Probes %u / %u, baked %u", GetProbeCount(), kMaxProbes, GetBakedCount());
	ImGui::Text("Bakes %u%s", bakeCount_, IsBaking() ? " (baking)" : "");
	ImGui::Checkbox("Auto rebake##probe", &isAutoRebake_);
	if (ImGui::Button("Rebake##probe")) {
		RequestRebake();
	}
	ImGui::TreePop();
#endif // _DEBUGMODE
}
