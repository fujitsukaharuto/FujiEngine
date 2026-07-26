#include "Engine/GraphicPipeline/PipelineManager.h"
#include <string>

#include "Engine/DX/DXCom.h"
#include "Engine/GraphicPipeline/Pipeline.h"
#include "Engine/GraphicPipeline/Line3dPipe.h"
#include "Engine/GraphicPipeline/SpritePipe.h"
#include "Engine/GraphicPipeline/ParticlePipeline.h"
#include "Engine/GraphicPipeline/ParticleCSPipe.h"
#include "Engine/GraphicPipeline/AnimationPipeline.h"
#include "Engine/GraphicPipeline/SkyboxPipe.h"
#include "Engine/GraphicPipeline/MetaBallPipeline.h"
#include "Engine/GraphicPipeline/NonePipeline.h"
#include "Engine/GraphicPipeline/ShockWavePipe.h"
#include "Engine/GraphicPipeline/FirePipe.h"
#include "Engine/GraphicPipeline/ThunderPipe.h"
#include "Engine/GraphicPipeline/CRTPipe.h"
#include "Engine/GraphicPipeline/BaseGridPipe.h"
#include "Engine/GraphicPipeline/ComputePipeline.h"
#include "Engine/GraphicPipeline/CSPipe/OutlineCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/TrailEmitCSPipe.h"
#include "Engine/GraphicPipeline/SkinningCSPipe.h"
#include "Engine/GraphicPipeline/SplatCompositePipe.h"

using namespace Graphics;
using namespace DXC;


PipelineManager::~PipelineManager() {
}

PipelineManager* PipelineManager::GetInstance() {
	static PipelineManager instance;
	return &instance;
}

void PipelineManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void PipelineManager::Finalize() {
	for (auto& pipe : pipelines_) {
		pipe.reset();
	}
	dxcommon_ = nullptr;
}

void PipelineManager::CreatePipeline() {

	CreatePipe<NonePipeline>(Pipe::None);
	CreatePipe<NonePipeline>(Pipe::GPUParticleSynthesis, [](NonePipeline& p) {
		p.SetIsAddMode(true);
		});
	CreatePipe<Pipeline>(Pipe::Normal);

	CreatePipe<Pipeline>(Pipe::NormalAdd, [](Pipeline& p) {
		p.SetIsAddMode(true);
		});


	CreatePipe<SpritePipe>(Pipe::Sprite);
	CreatePipe<Line3dPipe>(Pipe::Line3d);


	CreatePipe<ParticlePipeline>(Pipe::Particle);
	CreatePipe<ParticlePipeline>(Pipe::ParticleAlpha, [](auto& p) {
		p.SetBlendType(BlendType::ALPHA);
		});
	CreatePipe<ParticlePipeline>(Pipe::ParticleSub, [](auto& p) {
		p.SetBlendType(BlendType::SUBTRACT);
		});
	CreatePipe<ParticlePipeline>(Pipe::ParticleScreen, [](auto& p) {
		p.SetBlendType(BlendType::SCREEN);
		});
	CreatePipe<ParticlePipeline>(Pipe::ParticleMultiply, [](auto& p) {
		p.SetBlendType(BlendType::MULTIPLY);
		});
	CreatePipe<ParticlePipeline>(Pipe::ParticleSoftAdd, [](auto& p) {
		p.SetBlendType(BlendType::SOFT_ADD);
		});
	CreatePipe<ParticlePipeline>(Pipe::ParticlePreMulAlpha, [](auto& p) {
		p.SetBlendType(BlendType::PREMULTIPLIED_ALPHA);
		});


	CreatePipe<ParticleCSPipe>(Pipe::ParticleCS);
	CreatePipe<AnimationPipeline>(Pipe::Animation);
	CreatePipe<SkyboxPipe>(Pipe::Skybox);
	CreatePipe<BaseGridPipe>(Pipe::BaseGrid);

	CreateComputePipe(Pipe::GrayCS, L"CS/Grayscale.CS.hlsl");
	CreateComputePipe(Pipe::GaussCS, L"CS/Gaussian.CS.hlsl");
	CreateComputePipe(Pipe::BoxFilterCS, L"CS/BoxFilter.CS.hlsl");
	CreateComputePipe(Pipe::RadialCS, L"CS/RadialBlur.CS.hlsl");
	CreateComputePipe(Pipe::VignetteCS, L"CS/Vignette.CS.hlsl");
	CreateComputePipe(Pipe::CRTCS, L"CS/CRTEffect.CS.hlsl");
	CreateComputePipe(Pipe::RetroTVCS, L"CS/RetroTV.CS.hlsl");
	CreatePipe<OutlineCSPipe>(Pipe::OutlineCS);
	CreateComputePipe(Pipe::LuminanceOutlineCS, L"CS/LuminanceBasedOutline.CS.hlsl");
	CreateComputePipe(Pipe::BloomCS, L"CS/Bloom.CS.hlsl");
	CreateComputePipe(Pipe::RandomCS, L"CS/Random.CS.hlsl");
	CreatePipe<SkinningCSPipe>(Pipe::SkinningCS);

	CreateComputePipe(Pipe::InitParticleCS, L"CS/Engine/InitParticle.CS.hlsl");
	CreateComputePipe(Pipe::EmitParticleCS, L"CS/Engine/EmitParticle.CS.hlsl");
	CreateComputePipe(Pipe::EmitTexParticleCS, L"CS/Engine/EmitTexParticle.CS.hlsl");
	CreateComputePipe(Pipe::EmitSurfaceParticleCS, L"CS/Engine/EmitSurfaceParticle.CS.hlsl");
	CreateComputePipe(Pipe::UpdateParticleCS, L"CS/Engine/UpdateParticle.CS.hlsl");
	CreateComputePipe(Pipe::UpdateParticleSplatCS, L"CS/Engine/UpdateParticleSplat.CS.hlsl");
	CreateComputePipe(Pipe::ClearParticleColorCS, L"CS/Engine/ClearParticleColor.CS.hlsl");
	CreatePipe<TrailEmitCSPipe>(Pipe::TrailEmitCS);
	CreateComputePipe(Pipe::InitArgsCS, L"CS/Engine/InitArgs.CS.hlsl");
	CreateComputePipe(Pipe::AliveCountCS, L"CS/Engine/AliveParticleCount.CS.hlsl");

	CreateComputePipe(Pipe::SplatClearCS, L"CS/Engine/SplatClear.CS.hlsl");
	CreateComputePipe(Pipe::SplatParticleCS, L"CS/Engine/SplatParticle.CS.hlsl");
	CreatePipe<SplatCompositePipe>(Pipe::SplatComposite);
}

void PipelineManager::CreateComputePipe(Pipe type, const std::wstring& csPath) {
	auto pipe = std::make_unique<ComputePipeline>();
	pipe->SetShaderPath(csPath);
	pipe->Initialize(dxcommon_);
	pipelines_[static_cast<size_t>(type)] = std::move(pipe);
}

void PipelineManager::SetPipeline(Pipe type) {
	auto& pipe = pipelines_[static_cast<size_t>(type)];
	assert(pipe);
	pipe->SetPipelineState();
	currentPipeline_ = pipe.get();
}

void PipelineManager::SetCSPipeline(Pipe type, uint32_t index) {
	auto& pipe = pipelines_[static_cast<size_t>(type)];
	assert(pipe);
	pipe->SetPipelineCSState(index);
	currentPipeline_ = pipe.get();
}

ID3D12RootSignature* Graphics::PipelineManager::GetRootSignature(Pipe type) {
	return pipelines_[static_cast<int>(type)]->GetRootSignature();
}

void PipelineManager::SetGraphicsRootCBV(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address) {
	assert(currentPipeline_);
	list->SetGraphicsRootConstantBufferView(currentPipeline_->GetRootIndex(name), address);
}

void PipelineManager::SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
	assert(currentPipeline_);
	list->SetGraphicsRootDescriptorTable(currentPipeline_->GetRootIndex(name), handle);
}

void PipelineManager::SetComputeRootCBV(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address) {
	assert(currentPipeline_);
	list->SetComputeRootConstantBufferView(currentPipeline_->GetRootIndex(name), address);
}

void PipelineManager::SetComputeRootDescriptorTable(ID3D12GraphicsCommandList* list, const std::string& name, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
	assert(currentPipeline_);
	list->SetComputeRootDescriptorTable(currentPipeline_->GetRootIndex(name), handle);
}

void PipelineManager::SetComputeRoot32BitConstants(ID3D12GraphicsCommandList* list, const std::string& name, UINT numValues, const void* data, UINT offset) {
	assert(currentPipeline_);
	list->SetComputeRoot32BitConstants(currentPipeline_->GetRootIndex(name), numValues, data, offset);
}
