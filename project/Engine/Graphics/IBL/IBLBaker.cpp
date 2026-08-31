#include "Engine/Graphics/IBL/IBLBaker.h"

#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Texture/TextureManager.h"

using namespace DXC;
using namespace Graphics;

namespace {

	// 環境マップが HDR なので、焼いた結果も 1.0 を超える。8bit では白飛びする
	constexpr DXGI_FORMAT kCubeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// split-sum の第2項は係数と加算項の2値。ここだけ2チャンネルで足りる
	constexpr DXGI_FORMAT kLutFormat = DXGI_FORMAT_R16G16_FLOAT;

	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	constexpr uint32_t kThreadGroupSize = 8;
	constexpr uint32_t kCubeFaceCount = 6;

	// Model.cpp の BindEnvironment が引いているものと同じ環境マップ
	constexpr const char* kEnvironmentTextureName = "skyboxTexture.dds";

	uint32_t DispatchCount(uint32_t pixels) {
		return (pixels + kThreadGroupSize - 1) / kThreadGroupSize;
	}
}


void IBLBaker::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	CreateResources(dxcommon_->GetDevice());
}

void IBLBaker::CreateResources(ID3D12Device* device) {
	SRVManager* srvManager = SRVManager::GetInstance();

	irradiance_.resource = Helper::CreateTextureCube(device, kIrradianceSize_, kCubeFormat, 1,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState);
	prefiltered_.resource = Helper::CreateTextureCube(device, kPrefilteredSize_, kCubeFormat,
		kPrefilteredMipLevels_, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState);
	brdfLut_ = Helper::CreateTexture2D(device, kBRDFLutSize_, kBRDFLutSize_, kLutFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState, nullptr);

	// キューブとして読ませるかは SRV の張り方で決まる。書く側は配列としてしか張れない
	irradiance_.srvIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(irradiance_.srvIndex, irradiance_.resource.Get(), kCubeFormat, 1, true);
	prefiltered_.srvIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(prefiltered_.srvIndex, prefiltered_.resource.Get(), kCubeFormat,
		kPrefilteredMipLevels_, true);
	brdfLutSrvIndex_ = srvManager->Allocate();
	srvManager->CreateTextureSRV(brdfLutSrvIndex_, brdfLut_.Get(), kLutFormat, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC cubeUavDesc{};
	cubeUavDesc.Format = kCubeFormat;
	cubeUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	cubeUavDesc.Texture2DArray.FirstArraySlice = 0;
	cubeUavDesc.Texture2DArray.ArraySize = kCubeFaceCount;

	irradiance_.uavIndex[0] = srvManager->Allocate();
	cubeUavDesc.Texture2DArray.MipSlice = 0;
	device->CreateUnorderedAccessView(irradiance_.resource.Get(), nullptr, &cubeUavDesc,
		srvManager->GetCPUDescriptorHandle(irradiance_.uavIndex[0]));

	// 焼くのはミップごとに別ディスパッチなので、UAV もミップごとに要る
	for (uint32_t mip = 0; mip < kPrefilteredMipLevels_; mip++) {
		prefiltered_.uavIndex[mip] = srvManager->Allocate();
		cubeUavDesc.Texture2DArray.MipSlice = mip;
		device->CreateUnorderedAccessView(prefiltered_.resource.Get(), nullptr, &cubeUavDesc,
			srvManager->GetCPUDescriptorHandle(prefiltered_.uavIndex[mip]));
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC lutUavDesc{};
	lutUavDesc.Format = kLutFormat;
	lutUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	brdfLutUavIndex_ = srvManager->Allocate();
	device->CreateUnorderedAccessView(brdfLut_.Get(), nullptr, &lutUavDesc,
		srvManager->GetCPUDescriptorHandle(brdfLutUavIndex_));

	irradianceParamResource_ = Helper::CreateBufferResource(device, sizeof(IrradianceParam));
	irradianceParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&irradianceParamMapped_));

	prefilterParamResource_ = Helper::CreateBufferResource(device,
		kPrefilterParamStride_ * kPrefilteredMipLevels_);
	prefilterParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&prefilterParamMapped_));

	// プレビューは常に UAV 状態のまま置く。読むのは ImGui だけで、書くのも自分だけ
	D3D12_UNORDERED_ACCESS_VIEW_DESC previewUavDesc{};
	previewUavDesc.Format = kCubeFormat;
	previewUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	for (uint32_t i = 0; i < kPreviewCount_; i++) {
		preview_[i] = Helper::CreateTexture2D(device, kPreviewWidth_, kPreviewHeight_, kCubeFormat,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, kReadState, nullptr);

		previewSrvIndex_[i] = srvManager->Allocate();
		srvManager->CreateTextureSRV(previewSrvIndex_[i], preview_[i].Get(), kCubeFormat, 1, false);

		previewUavIndex_[i] = srvManager->Allocate();
		device->CreateUnorderedAccessView(preview_[i].Get(), nullptr, &previewUavDesc,
			srvManager->GetCPUDescriptorHandle(previewUavIndex_[i]));
	}

	previewParamResource_ = Helper::CreateBufferResource(device, kPrefilterParamStride_ * kPreviewCount_);
	previewParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&previewParamMapped_));
}

void IBLBaker::Finalize() {
	if (irradianceParamResource_) {
		irradianceParamMapped_ = nullptr;
		irradianceParamResource_.Reset();
	}
	if (prefilterParamResource_) {
		prefilterParamMapped_ = nullptr;
		prefilterParamResource_.Reset();
	}
	if (previewParamResource_) {
		previewParamMapped_ = nullptr;
		previewParamResource_.Reset();
	}
	for (uint32_t i = 0; i < kPreviewCount_; i++) {
		preview_[i].Reset();
	}
	brdfLut_.Reset();
	prefiltered_.resource.Reset();
	irradiance_.resource.Reset();
	dxcommon_ = nullptr;
}

void IBLBaker::EnsureBaked(ID3D12GraphicsCommandList* commandList) {
	if (isBaked_ || dxcommon_ == nullptr) {
		return;
	}

	// 今は TextureManager::Initialize の LoadSkyCube で先に読まれているが、
	// その順序に寄りかからずに済むよう、読めた最初のフレームで焼く形にしてある
	Texture* environment = TextureManager::GetInstance()->LoadTexture(kEnvironmentTextureName);
	if (environment == nullptr) {
		return;
	}

	dxcommon_->TransitionResource(irradiance_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dxcommon_->TransitionResource(prefiltered_.resource.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dxcommon_->TransitionResource(brdfLut_.Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	bakedSourceSize_ = static_cast<uint32_t>(environment->meta.width);

	// 3つとも書き先が別で、互いの結果を読まないのでディスパッチの間にバリアは要らない
	DispatchBRDFLut(commandList);
	DispatchIrradiance(commandList, environment->gpuHandle);
	DispatchPrefilter(commandList, environment->gpuHandle);

	dxcommon_->TransitionResource(irradiance_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	dxcommon_->TransitionResource(prefiltered_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	dxcommon_->TransitionResource(brdfLut_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);

	isBaked_ = true;
}

void IBLBaker::RenderPreview(ID3D12GraphicsCommandList* commandList) {
	if (!isPreviewEnabled_ || !isBaked_) {
		return;
	}

	Texture* environment = TextureManager::GetInstance()->LoadTexture(kEnvironmentTextureName);
	if (environment == nullptr) {
		return;
	}

	for (uint32_t i = 0; i < kPreviewCount_; i++) {
		dxcommon_->TransitionResource(preview_[i].Get(), kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	// 0=元の環境マップ 1=焼いた irradiance 2=焼いた prefiltered。
	// 0 と 2(ミップ0) がほぼ同じ絵にならないなら、焼く側か面の向きの取り方が疑わしい
	DispatchPreview(commandList, 0, environment->gpuHandle, previewMip_);
	DispatchPreview(commandList, 1, GetIrradianceSrvHandle(), 0.0f);
	DispatchPreview(commandList, 2, GetPrefilteredSrvHandle(), previewMip_);

	for (uint32_t i = 0; i < kPreviewCount_; i++) {
		dxcommon_->TransitionResource(preview_[i].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	}
}

void IBLBaker::DispatchPreview(ID3D12GraphicsCommandList* commandList, uint32_t index,
	D3D12_GPU_DESCRIPTOR_HANDLE source, float mipLevel) {

	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	PreviewParam param{};
	param.mipLevel = mipLevel;
	param.exposure = previewExposure_;

	const uint64_t offset = kPrefilterParamStride_ * index;
	memcpy(previewParamMapped_ + offset, &param, sizeof(PreviewParam));

	pPipeManager->SetCSPipeline(Pipe::CubePreviewCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kPreviewParam,
		previewParamResource_->GetGPUVirtualAddress() + offset);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kPreviewSource, source);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kPreviewOut,
		srvManager->GetGPUDescriptorHandle(previewUavIndex_[index]));

	commandList->Dispatch(DispatchCount(kPreviewWidth_), DispatchCount(kPreviewHeight_), 1);
}

void IBLBaker::DispatchBRDFLut(ID3D12GraphicsCommandList* commandList) {
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	pPipeManager->SetCSPipeline(Pipe::BRDFLutCS);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kBRDFLutOut,
		srvManager->GetGPUDescriptorHandle(brdfLutUavIndex_));

	commandList->Dispatch(DispatchCount(kBRDFLutSize_), DispatchCount(kBRDFLutSize_), 1);
}

void IBLBaker::DispatchIrradiance(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE sourceEnv) {
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	IrradianceParam param{};
	param.sourceSize = bakedSourceSize_;
	param.sampleCount = kIrradianceSampleCount_;
	*irradianceParamMapped_ = param;

	pPipeManager->SetCSPipeline(Pipe::IrradianceBakeCS);
	pPipeManager->SetComputeRootCBV(commandList, RootName::kIrradianceParam,
		irradianceParamResource_->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSourceEnv, sourceEnv);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kIrradianceOut,
		srvManager->GetGPUDescriptorHandle(irradiance_.uavIndex[0]));

	// z が面の番号。6面を1回のディスパッチで焼く
	commandList->Dispatch(DispatchCount(kIrradianceSize_), DispatchCount(kIrradianceSize_), kCubeFaceCount);
}

void IBLBaker::DispatchPrefilter(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE sourceEnv) {
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	pPipeManager->SetCSPipeline(Pipe::PrefilterEnvCS);
	pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kSourceEnv, sourceEnv);

	for (uint32_t mip = 0; mip < kPrefilteredMipLevels_; mip++) {
		PrefilterParam param{};
		// ミップ番号をそのまま粗さに割り当てる。引く側は roughness からこの逆でミップを選ぶ
		param.roughness = static_cast<float>(mip) / static_cast<float>(kPrefilteredMipLevels_ - 1);
		// 粗さ0は散らす先が1点しかないので、本数を掛けても同じ値が積まれるだけ
		param.sampleCount = (mip == 0) ? 1 : kPrefilterSampleCount_;
		param.sourceSize = bakedSourceSize_;

		const uint64_t offset = kPrefilterParamStride_ * mip;
		memcpy(prefilterParamMapped_ + offset, &param, sizeof(PrefilterParam));

		pPipeManager->SetComputeRootCBV(commandList, RootName::kPrefilterParam,
			prefilterParamResource_->GetGPUVirtualAddress() + offset);
		pPipeManager->SetComputeRootDescriptorTable(commandList, RootName::kPrefilteredOut,
			srvManager->GetGPUDescriptorHandle(prefiltered_.uavIndex[mip]));

		const uint32_t mipSize = kPrefilteredSize_ >> mip;
		commandList->Dispatch(DispatchCount(mipSize), DispatchCount(mipSize), kCubeFaceCount);
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBaker::GetIrradianceSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(irradiance_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBaker::GetPrefilteredSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(prefiltered_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBLBaker::GetBRDFLutSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(brdfLutSrvIndex_);
}

void IBLBaker::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("IBL")) {
		return;
	}

	ImGui::Text("baked      : %s", isBaked_ ? "yes" : "not yet");
	ImGui::Text("source     : %u x %u", bakedSourceSize_, bakedSourceSize_);
	ImGui::Text("irradiance : %u x %u x 6", kIrradianceSize_, kIrradianceSize_);
	ImGui::Text("prefiltered: %u x %u x 6, %u mips", kPrefilteredSize_, kPrefilteredSize_, kPrefilteredMipLevels_);

	ImGui::Text("BRDF LUT (x = N・V, y = roughness)");
	ImGui::Image(static_cast<ImTextureID>(GetBRDFLutSrvHandle().ptr), ImVec2(128.0f, 128.0f));

	ImGui::SeparatorText("Preview");
	ImGui::Checkbox("Enable preview", &isPreviewEnabled_);
	// HDR なので ImGui 上では 1.0 で頭打ちになる。下げると明るい所の模様が見える
	ImGui::SliderFloat("exposure##ibl", &previewExposure_, 0.01f, 2.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("mip##ibl", &previewMip_, 0.0f, static_cast<float>(kPrefilteredMipLevels_ - 1));

	if (isPreviewEnabled_) {
		const ImVec2 size(static_cast<float>(kPreviewWidth_), static_cast<float>(kPreviewHeight_));
		SRVManager* srvManager = SRVManager::GetInstance();

		ImGui::Text("Source (mip %.1f)", previewMip_);
		ImGui::Image(static_cast<ImTextureID>(srvManager->GetGPUDescriptorHandle(previewSrvIndex_[0]).ptr), size);
		ImGui::Text("Irradiance");
		ImGui::Image(static_cast<ImTextureID>(srvManager->GetGPUDescriptorHandle(previewSrvIndex_[1]).ptr), size);
		ImGui::Text("Prefiltered (mip %.1f)", previewMip_);
		ImGui::Image(static_cast<ImTextureID>(srvManager->GetGPUDescriptorHandle(previewSrvIndex_[2]).ptr), size);
	}

	ImGui::TreePop();
#endif // _DEBUGMODE
}
