#include "Engine/Graphics/SkyBox/SkyAtmosphere.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"

using namespace DXC;
using namespace Graphics;
using namespace Math;

namespace {

	// 空は 1.0 を超えるので浮動小数
	constexpr DXGI_FORMAT kLutFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	constexpr uint32_t kThreadGroupSize = 8;
	constexpr uint32_t kCubeFaceCount = 6;

	uint32_t DispatchCount(uint32_t pixels) {
		return (pixels + kThreadGroupSize - 1) / kThreadGroupSize;
	}
}


void SkyAtmosphere::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	CreateResources(dxcommon_->GetDevice());
}

SkyAtmosphere::LutTexture SkyAtmosphere::CreateLut(ID3D12Device* device, uint32_t width, uint32_t height) {
	SRVManager* srvManager = SRVManager::GetInstance();

	LutTexture lut;
	lut.resource = Helper::CreateTexture2D(device, static_cast<int32_t>(width), static_cast<int32_t>(height), kLutFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState, nullptr);
	lut.srvIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(lut.srvIndex, lut.resource.Get(), kLutFormat, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = kLutFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	lut.uavIndex = srvManager->Allocate();
	device->CreateUnorderedAccessView(lut.resource.Get(), nullptr, &uavDesc, srvManager->GetCPUDescriptorHandle(lut.uavIndex));
	return lut;
}

void SkyAtmosphere::CreateResources(ID3D12Device* device) {
	SRVManager* srvManager = SRVManager::GetInstance();

	transmittance_ = CreateLut(device, kTransmittanceWidth_, kTransmittanceHeight_);
	multiScatter_ = CreateLut(device, kMultiScatterSize_, kMultiScatterSize_);
	skyView_ = CreateLut(device, kSkyViewWidth_, kSkyViewHeight_);

	skyCube_ = Helper::CreateTextureCube(device, kSkyCubeSize_, kLutFormat, kSkyCubeMipLevels_,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState);
	skyCubeSrvIndex_ = srvManager->Allocate();
	srvManager->CreateTextureSRV(skyCubeSrvIndex_, skyCube_.Get(), kLutFormat, kSkyCubeMipLevels_, true);

	// 書く側は配列として張る。UAV はミップごと
	D3D12_UNORDERED_ACCESS_VIEW_DESC cubeUavDesc{};
	cubeUavDesc.Format = kLutFormat;
	cubeUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	cubeUavDesc.Texture2DArray.FirstArraySlice = 0;
	cubeUavDesc.Texture2DArray.ArraySize = kCubeFaceCount;
	for (uint32_t mip = 0; mip < kSkyCubeMipLevels_; mip++) {
		skyCubeUavIndex_[mip] = srvManager->Allocate();
		cubeUavDesc.Texture2DArray.MipSlice = mip;
		device->CreateUnorderedAccessView(skyCube_.Get(), nullptr, &cubeUavDesc,
			srvManager->GetCPUDescriptorHandle(skyCubeUavIndex_[mip]));
	}

	// 定数はフレーム数ぶん持つ
	paramResource_ = Helper::CreateBufferResource(device, kParamStride_ * kFrameCount_);
	paramResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_));

	summaryBuffer_ = Helper::CreateUAVResource(device, kSummaryBytes_);
	summaryUavIndex_ = srvManager->Allocate();
	srvManager->CreateStructuredUAV(summaryUavIndex_, summaryBuffer_.Get(), kSummaryCount, sizeof(float) * 4);

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		summaryReadback_[i] = Helper::CreateReadbackResource(device, kSummaryBytes_);
		// 読み戻し用は Map したまま
		void* mapped = nullptr;
		summaryReadback_[i]->Map(0, nullptr, &mapped);
		summaryMapped_[i] = static_cast<const float*>(mapped);
	}
}

void SkyAtmosphere::Finalize() {
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		if (summaryReadback_[i]) {
			summaryReadback_[i]->Unmap(0, nullptr);
			summaryReadback_[i].Reset();
		}
		summaryMapped_[i] = nullptr;
	}
	summaryBuffer_.Reset();
	if (paramResource_) {
		paramMapped_ = nullptr;
		paramResource_.Reset();
	}
	skyCube_.Reset();
	skyView_.resource.Reset();
	multiScatter_.resource.Reset();
	transmittance_.resource.Reset();
	dxcommon_ = nullptr;
}

void SkyAtmosphere::Render(ID3D12GraphicsCommandList* commandList, const Vector3& sunDirection,
	const Vector3& sunIlluminance, float cameraHeight, float cloudCoverage, float cloudTime) {
	if (dxcommon_ == nullptr) {
		return;
	}

	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	params_.sunDirection = sunDirection;
	params_.sunIlluminance = sunIlluminance;
	params_.cloudCoverage = cloudCoverage;
	params_.cloudTime = cloudTime;
	// 1 単位を 1m とみなす。カメラは大気の外へ出さない
	params_.cameraHeight = (std::max)(cameraHeight, 0.0f) * 0.001f;
	const uint64_t offset = kParamStride_ * frameIndex;
	std::memcpy(paramMapped_ + offset, &params_, sizeof(AtmosphereParams));
	const D3D12_GPU_VIRTUAL_ADDRESS paramAddress = paramResource_->GetGPUVirtualAddress() + offset;

	if (!isStaticLutReady_) {
		RenderStaticLuts(commandList, paramAddress);
		isStaticLutReady_ = true;
	}

	// カメラから見た空
	dxcommon_->TransitionResource(skyView_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::AtmosphereSkyViewCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kAtmosphereParams, paramAddress);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kTransmittanceLut,
		srvManager->GetGPUDescriptorHandle(transmittance_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kMultiScatterLut,
		srvManager->GetGPUDescriptorHandle(multiScatter_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSkyViewOut,
		srvManager->GetGPUDescriptorHandle(skyView_.uavIndex));
	commandList->Dispatch(DispatchCount(kSkyViewWidth_), DispatchCount(kSkyViewHeight_), 1);
	dxcommon_->TransitionResource(skyView_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	// キューブマップへ写す。ミップも数表から直接引く
	dxcommon_->TransitionResource(skyCube_.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::AtmosphereCaptureCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kAtmosphereParams, paramAddress);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSkyViewLut,
		srvManager->GetGPUDescriptorHandle(skyView_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kTransmittanceLut,
		srvManager->GetGPUDescriptorHandle(transmittance_.srvIndex));
	for (uint32_t mip = 0; mip < kSkyCubeMipLevels_; mip++) {
		pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSkyCubeOut,
			srvManager->GetGPUDescriptorHandle(skyCubeUavIndex_[mip]));
		const uint32_t mipSize = (std::max)(kSkyCubeSize_ >> mip, 1u);
		commandList->Dispatch(DispatchCount(mipSize), DispatchCount(mipSize), kCubeFaceCount);
	}
	dxcommon_->TransitionResource(skyCube_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	// CPU 側で使う値。書いたら読み戻し用へ写す
	dxcommon_->TransitionResource(summaryBuffer_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::AtmosphereSummaryCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kAtmosphereParams, paramAddress);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSkyViewLut,
		srvManager->GetGPUDescriptorHandle(skyView_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kTransmittanceLut,
		srvManager->GetGPUDescriptorHandle(transmittance_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kMultiScatterLut,
		srvManager->GetGPUDescriptorHandle(multiScatter_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSkyCube, GetSkyCubeSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSummaryOut,
		srvManager->GetGPUDescriptorHandle(summaryUavIndex_));
	commandList->Dispatch(1, 1, 1);
	dxcommon_->TransitionResource(summaryBuffer_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->CopyBufferRegion(summaryReadback_[frameIndex].Get(), 0, summaryBuffer_.Get(), 0, kSummaryBytes_);
	dxcommon_->TransitionResource(summaryBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
	isSummaryPending_[frameIndex] = true;

	UpdateSkyCubeVersion(sunDirection, sunIlluminance);
}

void SkyAtmosphere::RenderStaticLuts(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS params) {
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(transmittance_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::AtmosphereTransmittanceCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kAtmosphereParams, params);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kTransmittanceOut,
		srvManager->GetGPUDescriptorHandle(transmittance_.uavIndex));
	commandList->Dispatch(DispatchCount(kTransmittanceWidth_), DispatchCount(kTransmittanceHeight_), 1);
	dxcommon_->TransitionResource(transmittance_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	// 多重散乱は透過率の書き終わりを待つ
	dxcommon_->TransitionResource(multiScatter_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::AtmosphereMultiScatterCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kAtmosphereParams, params);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kTransmittanceLut,
		srvManager->GetGPUDescriptorHandle(transmittance_.srvIndex));
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kMultiScatterOut,
		srvManager->GetGPUDescriptorHandle(multiScatter_.uavIndex));
	commandList->Dispatch(DispatchCount(kMultiScatterSize_), DispatchCount(kMultiScatterSize_), 1);
	dxcommon_->TransitionResource(multiScatter_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
}

void SkyAtmosphere::UpdateSkyCubeVersion(const Vector3& sunDirection, const Vector3& sunIlluminance) {
	++framesSinceRebake_;
	const float previous = (std::max)({ bakedSunIlluminance_.x, bakedSunIlluminance_.y, bakedSunIlluminance_.z, 1e-6f });
	const Vector3 difference = sunIlluminance - bakedSunIlluminance_;
	const float change = (std::max)({ std::abs(difference.x), std::abs(difference.y), std::abs(difference.z) }) / previous;
	const bool isChanged = isRebakeForced_
		|| Vector3::Dot(sunDirection, bakedSunDirection_) < kRebakeAngleCos_
		|| change > kRebakeIlluminanceRatio_;
	if (isChanged && (isRebakeForced_ || framesSinceRebake_ >= kRebakeIntervalFrames_)) {
		++skyCubeVersion_;
		isRebakeForced_ = false;
		bakedSunDirection_ = sunDirection;
		bakedSunIlluminance_ = sunIlluminance;
		framesSinceRebake_ = 0;
	}
}

bool SkyAtmosphere::ReadSummary(uint32_t frameIndex, Summary& summary) {
	if (frameIndex >= kFrameCount_ || !isSummaryPending_[frameIndex] || summaryMapped_[frameIndex] == nullptr) {
		return false;
	}
	isSummaryPending_[frameIndex] = false;

	const float* data = summaryMapped_[frameIndex];
	auto read = [data](uint32_t index) {
		return Vector3{ data[index * 4 + 0], data[index * 4 + 1], data[index * 4 + 2] };
	};
	for (uint32_t i = 0; i < 4; i++) {
		summary.horizon[i] = read(kSummaryHorizon + i);
	}
	summary.ambientUp = read(kSummaryAmbientUp);
	summary.ambientDown = read(kSummaryAmbientDown);
	summary.sunTransmittance = read(kSummarySunTransmittance);
	summary.multiScattering = read(kSummaryMultiScattering);
	return true;
}

SkyAtmosphere::Medium SkyAtmosphere::GetMedium(float height) const {
	const float heightKm = (std::max)(height, 0.0f) * 0.001f;
	const float rayleighDensity = std::exp(params_.rayleighDensityScale * heightKm);
	const float mieDensity = std::exp(params_.mieDensityScale * heightKm);
	const float ozoneDensity = std::clamp(1.0f - std::abs(heightKm - params_.ozoneCenter) / params_.ozoneWidth, 0.0f, 1.0f);

	// 1km あたりから 1m あたりへ
	constexpr float kPerMeter = 0.001f;
	Medium medium;
	medium.rayleigh = params_.rayleighScattering * (rayleighDensity * kPerMeter);
	medium.mie = params_.mieScattering * (mieDensity * kPerMeter);
	medium.extinction = medium.rayleigh + params_.mieExtinction * (mieDensity * kPerMeter)
		+ params_.ozoneAbsorption * (ozoneDensity * kPerMeter);
	return medium;
}

D3D12_GPU_DESCRIPTOR_HANDLE SkyAtmosphere::GetSkyViewSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(skyView_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE SkyAtmosphere::GetSkyCubeSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(skyCubeSrvIndex_);
}

void SkyAtmosphere::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Sky Atmosphere")) {
		return;
	}

	ImGui::Text("ready : %s", isStaticLutReady_ ? "yes" : "not yet");
	ImGui::Text("cube version : %llu", static_cast<unsigned long long>(skyCubeVersion_));

	// 大気の性質を変えたら透過率と多重散乱を作り直す
	bool changed = false;
	changed |= ImGui::DragFloat3("Rayleigh (/km)", &params_.rayleighScattering.x, 0.0001f, 0.0f, 1.0f, "%.5f");
	changed |= ImGui::DragFloat3("Mie (/km)", &params_.mieScattering.x, 0.0001f, 0.0f, 1.0f, "%.5f");
	changed |= ImGui::DragFloat3("Mie extinction (/km)", &params_.mieExtinction.x, 0.0001f, 0.0f, 1.0f, "%.5f");
	changed |= ImGui::SliderFloat("Mie g", &params_.miePhaseG, 0.0f, 0.99f);
	changed |= ImGui::DragFloat3("Ozone (/km)", &params_.ozoneAbsorption.x, 0.0001f, 0.0f, 1.0f, "%.5f");
	changed |= ImGui::ColorEdit3("Ground albedo", &params_.groundAlbedo.x);
	if (changed) {
		isStaticLutReady_ = false;
		isRebakeForced_ = true;
	}

	ImGui::Text("Sky view");
	ImGui::Image(static_cast<ImTextureID>(GetSkyViewSrvHandle().ptr), ImVec2(192.0f, 108.0f));
	ImGui::Text("Transmittance");
	ImGui::Image(static_cast<ImTextureID>(SRVManager::GetInstance()->GetGPUDescriptorHandle(transmittance_.srvIndex).ptr),
		ImVec2(256.0f, 64.0f));

	ImGui::TreePop();
#endif // _DEBUGMODE
}
