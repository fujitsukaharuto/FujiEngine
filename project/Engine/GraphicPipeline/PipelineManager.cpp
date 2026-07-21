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
#include "Engine/GraphicPipeline/CSPipe/GrayCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/GaussCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/BoxFilterCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/RadialBlurCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/VignetteCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/CRTCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/RetroTVCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/OutlineCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/LuminanceOutlineCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/BloomCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/RandomCSPipe.h"
#include "Engine/GraphicPipeline/SkinningCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/InitParticleCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/EmitterParticleCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/EmitterTexParticleCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/EmitterSurfaceParticleCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/UpdateParticleCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/UpdateParticleSplatCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/ClearParticleColorCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/TrailEmitCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/InitArgsCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/AliveCountCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/SplatClearCSPipe.h"
#include "Engine/GraphicPipeline/CSPipe/SplatParticleCSPipe.h"
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

	CreatePipe<GrayCSPipe>(Pipe::GrayCS);
	CreatePipe<GaussCSPipe>(Pipe::GaussCS);
	CreatePipe<BoxFilterCSPipe>(Pipe::BoxFilterCS);
	CreatePipe<RadialBlurCSPipe>(Pipe::RadialCS);
	CreatePipe<VignetteCSPipe>(Pipe::VignetteCS);
	CreatePipe<CRTCSPipe>(Pipe::CRTCS);
	CreatePipe<RetroTVCSPipe>(Pipe::RetroTVCS);
	CreatePipe<OutlineCSPipe>(Pipe::OutlineCS);
	CreatePipe<LuminanceOutlineCSPipe>(Pipe::LuminanceOutlineCS);
	CreatePipe<BloomCSPipe>(Pipe::BloomCS);
	CreatePipe<RandomCSPipe>(Pipe::RandomCS);
	CreatePipe<SkinningCSPipe>(Pipe::SkinningCS);

	CreatePipe<InitParticleCSPipe>(Pipe::InitParticleCS);
	CreatePipe<EmitterParticleCSPipe>(Pipe::EmitParticleCS);
	CreatePipe<EmitterTexParticleCSPipe>(Pipe::EmitTexParticleCS);
	CreatePipe<EmitterSurfaceParticleCSPipe>(Pipe::EmitSurfaceParticleCS);
	CreatePipe<UpdateParticleCSPipe>(Pipe::UpdateParticleCS);
	CreatePipe<UpdateParticleSplatCSPipe>(Pipe::UpdateParticleSplatCS);
	CreatePipe<ClearParticleColorCSPipe>(Pipe::ClearParticleColorCS);
	CreatePipe<TrailEmitCSPipe>(Pipe::TrailEmitCS);
	CreatePipe<InitArgsCSPipe>(Pipe::InitArgsCS);
	CreatePipe<AliveCountCSPipe>(Pipe::AliveCountCS);

	CreatePipe<SplatClearCSPipe>(Pipe::SplatClearCS);
	CreatePipe<SplatParticleCSPipe>(Pipe::SplatParticleCS);
	CreatePipe<SplatCompositePipe>(Pipe::SplatComposite);
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
