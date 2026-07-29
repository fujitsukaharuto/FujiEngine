#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include <string>
#include <utility>

#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Pipeline/Render/RenderPipeline.h"
#include "Engine/Graphics/Pipeline/Compute/ParticleCSPipe.h"
#include "Engine/Graphics/Pipeline/Compute/ComputePipeline.h"
#include "Engine/Graphics/Pipeline/Compute/OutlineCSPipe.h"
#include "Engine/Graphics/Pipeline/Compute/TrailEmitCSPipe.h"
#include "Engine/Graphics/Pipeline/Compute/SkinningCSPipe.h"
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

	// オフスクリーンをそのまま画面へ出すフルスクリーンパス
	CreateRenderPipe(Pipe::None, {
		.vsPath = L"NonePost.VS.hlsl", .psPath = L"NonePost.PS.hlsl",
		.blend = BlendType::NONE, .depth = DepthMode::DISABLE,
		.cull = D3D12_CULL_MODE_BACK });
	// 1/4解像度GPUパーティクルRTをシーンへ加算合成する
	CreateRenderPipe(Pipe::GPUParticleSynthesis, {
		.vsPath = L"NonePost.VS.hlsl", .psPath = L"NonePost.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::DISABLE,
		.cull = D3D12_CULL_MODE_BACK });

	CreateRenderPipe(Pipe::Normal, {
		.vsPath = L"Object3d.VS.hlsl", .psPath = L"Object3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE });
	CreateRenderPipe(Pipe::NormalAdd, {
		.vsPath = L"Object3d.VS.hlsl", .psPath = L"Object3d.PS.hlsl",
		.blend = BlendType::ADD, .depth = DepthMode::READ_ONLY });


	CreateRenderPipe(Pipe::Sprite, {
		.vsPath = L"Object3d.VS.hlsl", .psPath = L"Sprite.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE });
	CreateRenderPipe(Pipe::Line3d, {
		.vsPath = L"Line3d.VS.hlsl", .psPath = L"Line3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::DISABLE,
		.topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE });


	// パーティクルは合成モードだけが違う
	const std::pair<Pipe, BlendType> kParticlePipes[] = {
		{ Pipe::Particle,            BlendType::ADD },
		{ Pipe::ParticleAlpha,       BlendType::ALPHA },
		{ Pipe::ParticleSub,         BlendType::SUBTRACT },
		{ Pipe::ParticleScreen,      BlendType::SCREEN },
		{ Pipe::ParticleMultiply,    BlendType::MULTIPLY },
		{ Pipe::ParticleSoftAdd,     BlendType::SOFT_ADD },
		{ Pipe::ParticlePreMulAlpha, BlendType::PREMULTIPLIED_ALPHA },
	};
	for (const auto& [type, blend] : kParticlePipes) {
		CreateRenderPipe(type, {
			.vsPath = L"Particle.VS.hlsl", .psPath = L"Particle.PS.hlsl",
			.blend = blend, .depth = DepthMode::READ_ONLY });
	}


	CreatePipe<ParticleCSPipe>(Pipe::ParticleCS);
	CreateRenderPipe(Pipe::Animation, {
		.vsPath = L"Object3d.VS.hlsl", .psPath = L"EnvMapObject3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE });
	CreateRenderPipe(Pipe::Skybox, {
		.vsPath = L"Skybox.VS.hlsl", .psPath = L"Skybox.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_ONLY });
	// SV_VertexID でフルスクリーン三角形を作るので頂点入力は無い
	CreateRenderPipe(Pipe::BaseGrid, {
		.vsPath = L"BaseGrid.VS.hlsl", .psPath = L"BaseGrid.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE,
		.useInputLayout = false });

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
	// 蓄積バッファを加算合成で重ねるだけなので深度も頂点入力も使わない
	CreateRenderPipe(Pipe::SplatComposite, {
		.vsPath = L"SplatComposite.VS.hlsl", .psPath = L"SplatComposite.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::DISABLE,
		.useInputLayout = false, .useDepthTarget = false });
}

void PipelineManager::CreateRenderPipe(Pipe type, const RenderPipelineDesc& desc) {
	auto pipe = std::make_unique<RenderPipeline>();
	pipe->SetDesc(desc);
	pipe->Initialize(dxcommon_);
	pipelines_[static_cast<size_t>(type)] = std::move(pipe);
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

