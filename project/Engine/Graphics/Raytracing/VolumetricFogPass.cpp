#include "Engine/Graphics/Raytracing/VolumetricFogPass.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Graphics/IBL/IBLBaker.h"
#include "Engine/Graphics/Light/ClusteredLightPass.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;
using namespace Math;

namespace {

	// 光は 1.0 を超えるので浮動小数。a は消散 (散らす側) と透過率 (積んだ側)
	constexpr DXGI_FORMAT kVolumeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	// UE と同じく、カメラのすぐ前には段を置かない (UE の NearOffset 9.5cm)
	constexpr float kNearOffset = 0.095f;

	uint32_t DispatchCount(uint32_t count, uint32_t groupSize) {
		return (count + groupSize - 1) / groupSize;
	}

	/// <summary>Halton 列。1番目から返す (0 は箱の角に偏るので使わない)</summary>
	float Halton(uint32_t index, uint32_t base) {
		float fraction = 1.0f;
		float result = 0.0f;
		for (uint32_t i = index + 1; i > 0; i /= base) {
			fraction /= static_cast<float>(base);
			result += fraction * static_cast<float>(i % base);
		}
		return result;
	}
}


void VolumetricFogPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();

	gridWidth_ = DispatchCount(static_cast<uint32_t>(MyWin::kWindowWidth), kGridPixelSize_);
	gridHeight_ = DispatchCount(static_cast<uint32_t>(MyWin::kWindowHeight), kGridPixelSize_);
	scatter_[0] = CreateVolume(device);
	scatter_[1] = CreateVolume(device);
	integrated_ = CreateVolume(device);

	// enabled = 0 から始める
	paramResource_ = Helper::CreateBufferResource(device, kParamStride_ * kFrameCount_);
	paramResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_));
	std::memset(paramMapped_, 0, kParamStride_ * kFrameCount_);

	// 局所の霧はフレームごとに書くので、フレーム数ぶん持って書いたままにする
	SRVManager* srvManager = SRVManager::GetInstance();
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		localFogResource_[i] = Helper::CreateBufferResource(device, sizeof(LocalFogVolumeGPU) * kMaxLocalFogVolumes);
		localFogResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&localFogMapped_[i]));
		std::memset(localFogMapped_[i], 0, sizeof(LocalFogVolumeGPU) * kMaxLocalFogVolumes);
		localFogSrvIndex_[i] = srvManager->Allocate();
		srvManager->CreateStructuredSRV(localFogSrvIndex_[i], localFogResource_[i].Get(), kMaxLocalFogVolumes, sizeof(LocalFogVolumeGPU));
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), kTimer_Count);
	gpuTimer_.SetName(kTimer_Scatter, "  Volumetric fog scatter");
	gpuTimer_.SetName(kTimer_Integrate, "  Volumetric fog integrate");
}

void VolumetricFogPass::Finalize() {
	gpuTimer_.Finalize();
	if (paramResource_) {
		paramMapped_ = nullptr;
		paramResource_.Reset();
	}
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		localFogMapped_[i] = nullptr;
		localFogResource_[i].Reset();
	}
	scatter_[0].resource.Reset();
	scatter_[1].resource.Reset();
	integrated_.resource.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

VolumetricFogPass::VolumeTexture VolumetricFogPass::CreateVolume(ID3D12Device* device) {
	SRVManager* srvManager = SRVManager::GetInstance();

	VolumeTexture volume;
	volume.resource = Helper::CreateTexture3D(device, gridWidth_, gridHeight_, kGridSizeZ_, kVolumeFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = kVolumeFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MipLevels = 1;
	volume.srvIndex = srvManager->Allocate();
	device->CreateShaderResourceView(volume.resource.Get(), &srvDesc, srvManager->GetCPUDescriptorHandle(volume.srvIndex));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = kVolumeFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.WSize = kGridSizeZ_;
	volume.uavIndex = srvManager->Allocate();
	device->CreateUnorderedAccessView(volume.resource.Get(), nullptr, &uavDesc, srvManager->GetCPUDescriptorHandle(volume.uavIndex));
	return volume;
}

Vector3 VolumetricFogPass::MakeGridZParams(float nearDepth, float farDepth) {
	// 段 = log2(奥行き × B + O) × S (UE の GetVolumetricFogGridZParams)。
	// 段 0 を near に、最後の段の中心 (段数 - 0.5) を far に置く。
	// UE は最後の段の手前の端 (段数 - 1) を far に置くので、表の最後と式の霧が 1段の半分ほど重なる
	const double S = kDepthDistributionScale_;
	const double N = nearDepth;
	const double F = farDepth;
	const double E = std::exp2((static_cast<double>(kGridSizeZ_) - 0.5) / S);
	const double O = (F - N * E) / (F - N);
	const double B = (1.0 - O) / N;
	return { static_cast<float>(B), static_cast<float>(O), static_cast<float>(S) };
}

void VolumetricFogPass::SetLocalFogVolumes(const std::vector<LocalFogVolumeDesc>& volumes) {
	localFogVolumes_.assign(volumes.begin(), volumes.begin() + (std::min)(volumes.size(), static_cast<size_t>(kMaxLocalFogVolumes)));
}

uint32_t VolumetricFogPass::UploadLocalFogVolumes(uint32_t frameIndex, const Vector3& cameraPosition) {
	// 奥から手前へ重ねる (UE と同じく中心までの距離で並べる)
	std::vector<const LocalFogVolumeDesc*> sorted;
	sorted.reserve(localFogVolumes_.size());
	for (const LocalFogVolumeDesc& volume : localFogVolumes_) {
		const bool isVisible = volume.radialExtinction > 0.0f || volume.heightExtinction > 0.0f;
		const bool hasSize = std::abs(volume.radii.x) > 1e-3f && std::abs(volume.radii.y) > 1e-3f && std::abs(volume.radii.z) > 1e-3f;
		if (isVisible && hasSize) {
			sorted.push_back(&volume);
		}
	}
	std::sort(sorted.begin(), sorted.end(), [&cameraPosition](const LocalFogVolumeDesc* a, const LocalFogVolumeDesc* b) {
		return (a->center - cameraPosition).Length() > (b->center - cameraPosition).Length();
	});

	LocalFogVolumeGPU* out = localFogMapped_[frameIndex];
	for (size_t i = 0; i < sorted.size(); i++) {
		const LocalFogVolumeDesc& volume = *sorted[i];
		LocalFogVolumeGPU gpu;
		gpu.worldToUnit = Inverse(MakeAffineMatrix(volume.radii, volume.rotation, volume.center));
		gpu.albedo = volume.albedo;
		gpu.radialExtinction = (std::max)(volume.radialExtinction, 0.0f);
		gpu.emissive = volume.emissive;
		gpu.heightExtinction = (std::max)(volume.heightExtinction, 0.0f);
		gpu.heightFalloff = volume.heightFalloff;
		gpu.baseHeight = volume.center.y + volume.heightOffset;
		gpu.phaseG = std::clamp(volume.phaseG, -0.99f, 0.99f);
		out[i] = gpu;
	}
	return static_cast<uint32_t>(sorted.size());
}

bool VolumetricFogPass::CanRender(const RaytracingScene* scene, const ClusteredLightPass* clustered) const {
	return scene != nullptr && clustered != nullptr && scene->IsAvailable() &&
		scene->GetTlasSrvIndex() != RaytracingScene::kInvalidSrvIndex &&
		scene->GetInstanceDataSrvIndex() != RaytracingScene::kInvalidSrvIndex &&
		ObjectRenderer::GetInstance()->GetIBLBaker() != nullptr;
}

void VolumetricFogPass::Render(const RaytracingScene* scene, const ClusteredLightPass* clustered, const Camera& camera) {
	isRendered_ = false;
	if (dxcommon_ == nullptr || lightManager_ == nullptr) {
		return;
	}
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	const AllLightsData& lights = lightManager_->GetData();
	const FogData& fog = lights.fog;
	const uint32_t localFogCount = UploadLocalFogVolumes(frameIndex, camera.GetTranslate());
	localFogVolumeCount_ = localFogCount;

	// 段は霧の始まりから。画面の隅は奥行きより距離が長いので、隅でも始まりに届くよう手前へ寄せる
	const Matrix4x4& projection = camera.GetProjectionMatrix();
	const float tanX = 1.0f / projection.m[0][0];
	const float tanY = 1.0f / projection.m[1][1];
	const float cornerCos = 1.0f / std::sqrt(1.0f + tanX * tanX + tanY * tanY);
	// 局所の霧は霧の始まりより手前にも置けるので、そのときはカメラの前から
	const float nearDepth = (localFogCount > 0) ? camera.GetNearClip() + kNearOffset :
		(std::max)(camera.GetNearClip() + kNearOffset, fog.startDistance * cornerCos);
	const float farDepth = (std::min)(settings_.viewDistance, camera.GetFarClip());

	const bool hasFog = fog.density > 0.0f || fog.secondDensity > 0.0f;
	const bool hasEmissive = settings_.emissive.x > 0.0f || settings_.emissive.y > 0.0f || settings_.emissive.z > 0.0f;
	const bool isActive = settings_.enabled && (hasFog || hasEmissive || localFogCount > 0) && farDepth > nearDepth + 1.0f &&
		CanRender(scene, clustered);

	VolumetricFogParams params{};
	const Matrix4x4& viewProj = camera.GetViewProjectionMatrix();
	params.enabled = isActive ? 1u : 0u;
	// 局所の霧を式で積むときの光。表を作らないフレームも要る
	params.localFogVolumeCount = localFogCount;
	if (lights.numDirectionalLights > 0) {
		const DirectionalLight& sun = lights.directionalLights[0];
		params.lightDirection = Vector3::Normalize(sun.direction * -1.0f);
		params.lightRadiance = { sun.color.x * sun.intensity, sun.color.y * sun.intensity, sun.color.z * sun.intensity };
	}
	params.ambientSky = lights.ambientSkyColor * lights.ambientIntensity;
	params.ambientGround = lights.ambientGroundColor * lights.ambientIntensity;
	if (!isActive) {
		hasHistory_ = false;
		std::memcpy(paramMapped_ + kParamStride_ * frameIndex, &params, sizeof(VolumetricFogParams));
		return;
	}

	const Vector3 gridZParams = MakeGridZParams(nearDepth, farDepth);
	// 段の並びが変わったら、前のフレームの値は別の場所の値になる
	const bool isSameGrid = std::abs(gridZParams.x - prevGridZParams_.x) <= 1e-6f * std::abs(gridZParams.x) &&
		std::abs(gridZParams.y - prevGridZParams_.y) <= 1e-6f;
	const bool useHistory = settings_.temporal && hasHistory_ && isSameGrid;

	const Matrix4x4& world = camera.GetWorldMatrix();
	Vector3 forward = { world.m[2][0], world.m[2][1], world.m[2][2] };
	forward = forward * (1.0f / (std::max)(forward.Length(), 1e-6f));

	params.invViewProj = Inverse(viewProj);
	params.prevViewProj = useHistory ? prevViewProj_ : viewProj;
	params.cameraPosition = camera.GetTranslate();
	params.cameraForward = forward;
	params.maxDistance = farDepth;
	params.gridZParams = gridZParams;
	params.historyWeight = useHistory ? kHistoryWeight_ : 0.0f;
	params.gridSize[0] = gridWidth_;
	params.gridSize[1] = gridHeight_;
	params.gridSize[2] = kGridSizeZ_;
	params.screenToGridUV = {
		static_cast<float>(MyWin::kWindowWidth) / static_cast<float>(gridWidth_ * kGridPixelSize_),
		static_cast<float>(MyWin::kWindowHeight) / static_cast<float>(gridHeight_ * kGridPixelSize_) };
	params.invScreenSize = { 1.0f / static_cast<float>(MyWin::kWindowWidth), 1.0f / static_cast<float>(MyWin::kWindowHeight) };
	// 箱の中の位置は Halton 列で毎フレームずらす (UE の VolumetricFogTemporalRandom)。混ぜないなら中心
	params.jitter = settings_.temporal
		? Vector3{ Halton(jitterIndex_ & 1023u, 2), Halton(jitterIndex_ & 1023u, 3), Halton(jitterIndex_ & 1023u, 5) }
		: Vector3{ 0.5f, 0.5f, 0.5f };
	params.phaseG = std::clamp(settings_.scatteringDistribution, -0.99f, 0.99f);
	params.albedo = settings_.albedo;
	params.extinctionScale = (std::max)(settings_.extinctionScale, 0.0f);
	params.emissive = settings_.emissive;
	params.sunScale = (std::max)(settings_.sunScale, 0.0f);
	params.skyLightScale = (std::max)(settings_.skyLightScale, 0.0f);
	params.localLightScale = (std::max)(settings_.localLightScale, 0.0f);
	std::memcpy(paramMapped_ + kParamStride_ * frameIndex, &params, sizeof(VolumetricFogParams));

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const D3D12_GPU_VIRTUAL_ADDRESS paramAddress = GetParamsGPUVirtualAddress();
	const uint32_t groupsX = DispatchCount(gridWidth_, kThreadGroupSize_);
	const uint32_t groupsY = DispatchCount(gridHeight_, kThreadGroupSize_);
	const VolumeTexture& current = scatter_[scatterIndex_];
	const VolumeTexture& history = scatter_[scatterIndex_ ^ 1u];

	// 箱ごとの霧の濃さと光
	gpuTimer_.Begin(cList, frameIndex, kTimer_Scatter);
	dxcommon_->TransitionResource(current.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::VolumetricFogScatterCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kLights, lightManager_->GetLightsGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(cList, RootName::kVolumetricFog, paramAddress);
	pPipeManager->SetComputeRootCBV(cList, RootName::kClusterParams, clustered->GetParamsGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusteredLights, clustered->GetLightsSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusterCounts, clustered->GetCountsSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusterIndices, clustered->GetIndicesSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene->GetTlasSrvIndex()));
	// 抜きのある物の影が引く見た目。頂点とインデックスとテクスチャはヒープの先頭から番号で引く
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayInstances,
		srvManager->GetGPUDescriptorHandle(scene->GetInstanceDataSrvIndex()));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayVertices, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayIndices, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRayTextures, srvManager->GetGPUDescriptorHandle(0));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kIrradianceMap,
		ObjectRenderer::GetInstance()->GetIBLBaker()->GetIrradianceSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kLocalFogVolumes, GetLocalFogVolumesSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricFogHistory, srvManager->GetGPUDescriptorHandle(history.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricFogScatterOut, srvManager->GetGPUDescriptorHandle(current.uavIndex));
	cList->Dispatch(groupsX, groupsY, kGridSizeZ_);
	dxcommon_->TransitionResource(current.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Scatter);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Scatter);

	// 手前から積む
	gpuTimer_.Begin(cList, frameIndex, kTimer_Integrate);
	dxcommon_->TransitionResource(integrated_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::VolumetricFogIntegrateCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kVolumetricFog, paramAddress);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricFogScatter, srvManager->GetGPUDescriptorHandle(current.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kVolumetricFogOut, srvManager->GetGPUDescriptorHandle(integrated_.uavIndex));
	cList->Dispatch(groupsX, groupsY, 1);
	dxcommon_->TransitionResource(integrated_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	gpuTimer_.End(cList, frameIndex, kTimer_Integrate);
	gpuTimer_.Resolve(cList, frameIndex, kTimer_Integrate);

	prevViewProj_ = viewProj;
	prevGridZParams_ = gridZParams;
	hasHistory_ = true;
	scatterIndex_ ^= 1u;
	++jitterIndex_;
	isRendered_ = true;
}

D3D12_GPU_VIRTUAL_ADDRESS VolumetricFogPass::GetParamsGPUVirtualAddress() const {
	return paramResource_->GetGPUVirtualAddress() + kParamStride_ * dxcommon_->GetNowFrameCount();
}

D3D12_GPU_DESCRIPTOR_HANDLE VolumetricFogPass::GetLocalFogVolumesSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(localFogSrvIndex_[dxcommon_->GetNowFrameCount()]);
}

D3D12_GPU_DESCRIPTOR_HANDLE VolumetricFogPass::GetVolumeSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(integrated_.srvIndex);
}

void VolumetricFogPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Volumetric fog")) {
		return;
	}
	const uint32_t frameIndex = dxcommon_ ? dxcommon_->GetNowFrameCount() : 0;
	const uint32_t finishedFrame = (frameIndex + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("grid      : %u x %u x %u", gridWidth_, gridHeight_, kGridSizeZ_);
	ImGui::Text("rendered  : %s", isRendered_ ? "yes" : "no");
	ImGui::Text("local fog : %zu / %u", localFogVolumes_.size(), kMaxLocalFogVolumes);
	ImGui::Text("scatter   : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Scatter));
	ImGui::Text("integrate : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Integrate));
	ImGui::TreePop();
#endif // _DEBUGMODE
}
