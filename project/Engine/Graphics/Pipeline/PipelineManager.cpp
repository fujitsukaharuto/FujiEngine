#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include <string>

#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Pipeline/Render/Pipeline.h"
#include "Engine/Graphics/Pipeline/Render/Line3dPipe.h"
#include "Engine/Graphics/Pipeline/Render/SpritePipe.h"
#include "Engine/Graphics/Pipeline/Render/ParticlePipeline.h"
#include "Engine/Graphics/Pipeline/Compute/ParticleCSPipe.h"
#include "Engine/Graphics/Pipeline/Render/AnimationPipeline.h"
#include "Engine/Graphics/Pipeline/Render/SkyboxPipe.h"
#include "Engine/Graphics/Pipeline/Render/NonePipeline.h"
#include "Engine/Graphics/Pipeline/Render/BaseGridPipe.h"
#include "Engine/Graphics/Pipeline/Compute/ComputePipeline.h"
#include "Engine/Graphics/Pipeline/Compute/OutlineCSPipe.h"
#include "Engine/Graphics/Pipeline/Compute/TrailEmitCSPipe.h"
#include "Engine/Graphics/Pipeline/Compute/SkinningCSPipe.h"
#include "Engine/Graphics/Pipeline/Render/SplatCompositePipe.h"
#include "Engine/Graphics/Pipeline/RootParam.h"

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
	currentPipeType_ = type;
}

void PipelineManager::SetCSPipeline(Pipe type, uint32_t index) {
	auto& pipe = pipelines_[static_cast<size_t>(type)];
	assert(pipe);
	pipe->SetPipelineCSState(index);
	currentPipeline_ = pipe.get();
	currentPipeType_ = type;
}

ID3D12RootSignature* Graphics::PipelineManager::GetRootSignature(Pipe type) {
	return pipelines_[static_cast<int>(type)]->GetRootSignature();
}

void PipelineManager::SetGraphicsRootCBV(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_VIRTUAL_ADDRESS address) {
	assert(currentPipeline_);
	list->SetGraphicsRootConstantBufferView(param.Resolve(currentPipeType_, currentPipeline_), address);
}

void PipelineManager::SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
	assert(currentPipeline_);
	list->SetGraphicsRootDescriptorTable(param.Resolve(currentPipeType_, currentPipeline_), handle);
}

void PipelineManager::SetComputeRootCBV(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_VIRTUAL_ADDRESS address) {
	assert(currentPipeline_);
	list->SetComputeRootConstantBufferView(param.Resolve(currentPipeType_, currentPipeline_), address);
}

void PipelineManager::SetComputeRootDescriptorTable(ID3D12GraphicsCommandList* list, const RootParam& param, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
	assert(currentPipeline_);
	list->SetComputeRootDescriptorTable(param.Resolve(currentPipeType_, currentPipeline_), handle);
}

void PipelineManager::SetComputeRoot32BitConstants(ID3D12GraphicsCommandList* list, const RootParam& param, UINT numValues, const void* data, UINT offset) {
	assert(currentPipeline_);
	list->SetComputeRoot32BitConstants(param.Resolve(currentPipeType_, currentPipeline_), numValues, data, offset);
}

