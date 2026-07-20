#include "PipelineManager.h"

#include "Engine/DX/DXCom.h"
#include "Pipeline.h"
#include "PipelineNode.h"
#include "Line3dPipe.h"
#include "SpritePipe.h"
#include "ParticlePipeline.h"
#include "ParticleCSPipe.h"
#include "AnimationPipeline.h"
#include "SkyboxPipe.h"
#include "MetaBallPipeline.h"
#include "NonePipeline.h"
#include "ShockWavePipe.h"
#include "FirePipe.h"
#include "ThunderPipe.h"
#include "CRTPipe.h"
#include "BaseGridPipe.h"
#include "CSPipe/GrayCSPipe.h"
#include "CSPipe/GaussCSPipe.h"
#include "CSPipe/BoxFilterCSPipe.h"
#include "CSPipe/RadialBlurCSPipe.h"
#include "CSPipe/VignetteCSPipe.h"
#include "CSPipe/CRTCSPipe.h"
#include "CSPipe/RetroTVCSPipe.h"
#include "CSPipe/OutlineCSPipe.h"
#include "CSPipe/LuminanceOutlineCSPipe.h"
#include "CSPipe/BloomCSPipe.h"
#include "CSPipe/RandomCSPipe.h"
#include "SkinningCSPipe.h"
#include "CSPipe/InitParticleCSPipe.h"
#include "CSPipe/EmitterParticleCSPipe.h"
#include "CSPipe/EmitterTexParticleCSPipe.h"
#include "CSPipe/EmitterSurfaceParticleCSPipe.h"
#include "CSPipe/UpdateParticleCSPipe.h"
#include "CSPipe/UpdateParticleSplatCSPipe.h"
#include "CSPipe/ClearParticleColorCSPipe.h"
#include "CSPipe/TrailEmitCSPipe.h"
#include "CSPipe/InitArgsCSPipe.h"
#include "CSPipe/AliveCountCSPipe.h"
#include "CSPipe/SplatClearCSPipe.h"
#include "CSPipe/SplatParticleCSPipe.h"
#include "SplatCompositePipe.h"

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


	CreatePipe<PipelineNode>(Pipe::NormalNode);
	CreatePipe<PipelineNode>(Pipe::NormalNodeAdd, [](PipelineNode& p) {
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
