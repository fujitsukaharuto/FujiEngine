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
#include "Engine/Core/Thread/ParallelFor.h"

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

	// オフスクリーンを画面(バックバッファ)へ出す最終パス。ここだけ出力先がスワップチェーン
	CreateRenderPipe(Pipe::None, {
		.vsPath = L"PostEffect/NonePost.VS.hlsl", .psPath = L"PostEffect/Tonemap.PS.hlsl",
		.blend = BlendType::NONE, .depth = DepthMode::DISABLE,
		.cull = D3D12_CULL_MODE_BACK,
		.rtvFormat = DXC::kSwapChainFormat });
	// 1/4解像度GPUパーティクルRTをシーンへ加算合成する
	CreateRenderPipe(Pipe::GPUParticleSynthesis, {
		.vsPath = L"PostEffect/NonePost.VS.hlsl", .psPath = L"PostEffect/NonePost.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::DISABLE,
		.cull = D3D12_CULL_MODE_BACK });

	// 深度と法線だけを書き出すプリパス。ブレンドすると法線が混ざるので合成なしで上書きする
	CreateRenderPipe(Pipe::DepthNormalPrepass, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/DepthNormal.PS.hlsl",
		.blend = BlendType::NONE, .depth = DepthMode::READ_WRITE,
		.rtvFormat = DXC::kGBufferNormalFormat });

	// クリックされた画素のオブジェクト番号を拾うパス。深度バッファは見ない。
	// 1回目で手前の深度を求め、2回目でその深度の持ち主が番号を書く
	CreateRenderPipe(Pipe::PickingDepth, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/PickingDepth.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::DISABLE });
	CreateRenderPipe(Pipe::PickingID, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/PickingID.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::DISABLE });

	CreateRenderPipe(Pipe::Normal, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/Object3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE });
	CreateRenderPipe(Pipe::NormalAdd, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/Object3d.PS.hlsl",
		.blend = BlendType::ADD, .depth = DepthMode::READ_ONLY });
	CreateRenderPipe(Pipe::NormalPrepassed, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/Object3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_ONLY });
	// 半透明。深度は書かず、表だけ描く
	CreateRenderPipe(Pipe::NormalTranslucent, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/Object3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_ONLY,
		.cull = D3D12_CULL_MODE_BACK });


	CreateRenderPipe(Pipe::Sprite, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Sprite/Sprite.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE });
	CreateRenderPipe(Pipe::Line3d, {
		.vsPath = L"Line/Line3d.VS.hlsl", .psPath = L"Line/Line3d.PS.hlsl",
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
			.vsPath = L"Particle/Particle.VS.hlsl", .psPath = L"Particle/Particle.PS.hlsl",
			.blend = blend, .depth = DepthMode::READ_ONLY });
	}


	CreatePipe<ParticleCSPipe>(Pipe::ParticleCS);
	CreateRenderPipe(Pipe::Animation, {
		.vsPath = L"Object/Object3d.VS.hlsl", .psPath = L"Object/Object3d.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE });
	CreateRenderPipe(Pipe::Skybox, {
		.vsPath = L"SkyBox/Skybox.VS.hlsl", .psPath = L"SkyBox/Skybox.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_ONLY });
	// SV_VertexID でフルスクリーン三角形を作るので頂点入力は無い
	CreateRenderPipe(Pipe::BaseGrid, {
		.vsPath = L"Debug/BaseGrid.VS.hlsl", .psPath = L"Debug/BaseGrid.PS.hlsl",
		.blend = BlendType::ALPHA, .depth = DepthMode::READ_WRITE,
		.useInputLayout = false });

	CreateComputePipe(Pipe::RayTracedAOCS, L"Raytracing/RayTracedAO.CS.hlsl");
	CreateComputePipe(Pipe::RayTracedShadowCS, L"Raytracing/RayTracedShadow.CS.hlsl");
	CreateComputePipe(Pipe::DenoiseTemporalCS, L"Raytracing/DenoiseTemporal.CS.hlsl");
	CreateComputePipe(Pipe::DenoiseSpatialCS, L"Raytracing/DenoiseSpatial.CS.hlsl");

	CreateComputePipe(Pipe::IrradianceBakeCS, L"IBL/IrradianceBake.CS.hlsl");
	CreateComputePipe(Pipe::PrefilterEnvCS, L"IBL/PrefilterEnv.CS.hlsl");
	CreateComputePipe(Pipe::BRDFLutCS, L"IBL/BRDFLut.CS.hlsl");
	CreateComputePipe(Pipe::CubePreviewCS, L"IBL/CubePreview.CS.hlsl");

	CreateComputePipe(Pipe::AtmosphereTransmittanceCS, L"SkyBox/AtmosphereTransmittance.CS.hlsl");
	CreateComputePipe(Pipe::AtmosphereMultiScatterCS, L"SkyBox/AtmosphereMultiScatter.CS.hlsl");
	CreateComputePipe(Pipe::AtmosphereSkyViewCS, L"SkyBox/AtmosphereSkyView.CS.hlsl");
	CreateComputePipe(Pipe::AtmosphereCaptureCS, L"SkyBox/AtmosphereCapture.CS.hlsl");
	CreateComputePipe(Pipe::AtmosphereSummaryCS, L"SkyBox/AtmosphereSummary.CS.hlsl");

	CreateComputePipe(Pipe::DDGITraceCS, L"Raytracing/DDGITrace.CS.hlsl");
	CreateComputePipe(Pipe::DDGIUpdateIrradianceCS, L"Raytracing/DDGIUpdateIrradiance.CS.hlsl");
	CreateComputePipe(Pipe::DDGIUpdateDistanceCS, L"Raytracing/DDGIUpdateDistance.CS.hlsl");
	CreateComputePipe(Pipe::DDGIRelocationCS, L"Raytracing/DDGIRelocation.CS.hlsl");
	CreateComputePipe(Pipe::ScreenProbeTraceCS, L"Raytracing/ScreenProbeTrace.CS.hlsl");
	CreateComputePipe(Pipe::ScreenProbeFilterCS, L"Raytracing/ScreenProbeFilter.CS.hlsl");
	CreateComputePipe(Pipe::ScreenProbeIntegrateCS, L"Raytracing/ScreenProbeIntegrate.CS.hlsl");
	CreateComputePipe(Pipe::ScreenProbeTemporalCS, L"Raytracing/ScreenProbeTemporal.CS.hlsl");
	CreateComputePipe(Pipe::ScreenReflectionTraceCS, L"Raytracing/ScreenReflectionTrace.CS.hlsl");
	CreateComputePipe(Pipe::ScreenReflectionResolveCS, L"Raytracing/ScreenReflectionResolve.CS.hlsl");
	CreateComputePipe(Pipe::VolumetricLightTraceCS, L"Raytracing/VolumetricLightTrace.CS.hlsl");
	CreateComputePipe(Pipe::VolumetricLightBlurCS, L"Raytracing/VolumetricLightBlur.CS.hlsl");
	CreateComputePipe(Pipe::ClusterLightCullCS, L"Light/ClusterLightCull.CS.hlsl");
	CreateComputePipe(Pipe::RainDripFindCS, L"Particle/RainDripFind.CS.hlsl");

	CreateComputePipe(Pipe::GrayCS, L"PostEffect/Grayscale.CS.hlsl");
	CreateComputePipe(Pipe::GaussCS, L"PostEffect/Gaussian.CS.hlsl");
	CreateComputePipe(Pipe::BoxFilterCS, L"PostEffect/BoxFilter.CS.hlsl");
	CreateComputePipe(Pipe::RadialCS, L"PostEffect/RadialBlur.CS.hlsl");
	CreateComputePipe(Pipe::VignetteCS, L"PostEffect/Vignette.CS.hlsl");
	CreateComputePipe(Pipe::CRTCS, L"PostEffect/CRTEffect.CS.hlsl");
	CreateComputePipe(Pipe::RetroTVCS, L"PostEffect/RetroTV.CS.hlsl");
	CreatePipe<OutlineCSPipe>(Pipe::OutlineCS);
	CreateComputePipe(Pipe::LuminanceOutlineCS, L"PostEffect/LuminanceBasedOutline.CS.hlsl");
	CreateComputePipe(Pipe::BloomCS, L"PostEffect/Bloom.CS.hlsl");
	CreateComputePipe(Pipe::BloomDownsampleCS, L"PostEffect/BloomDownsample.CS.hlsl");
	CreateComputePipe(Pipe::BloomUpsampleCS, L"PostEffect/BloomUpsample.CS.hlsl");
	CreateComputePipe(Pipe::BloomCompositeCS, L"PostEffect/BloomComposite.CS.hlsl");
	CreateComputePipe(Pipe::AutoExposureHistogramCS, L"PostEffect/AutoExposureHistogram.CS.hlsl");
	CreateComputePipe(Pipe::AutoExposureAverageCS, L"PostEffect/AutoExposureAverage.CS.hlsl");
	CreateComputePipe(Pipe::LocalTonemapDownsampleCS, L"PostEffect/LocalTonemapDownsample.CS.hlsl");
	CreateComputePipe(Pipe::LocalTonemapBlurCS, L"PostEffect/LocalTonemapBlur.CS.hlsl");
	CreateComputePipe(Pipe::BufferVisualizeCS, L"PostEffect/BufferVisualize.CS.hlsl");
	CreateComputePipe(Pipe::BakedGITraceCS, L"Raytracing/BakedGITrace.CS.hlsl");
	CreateComputePipe(Pipe::ReflectionProbeCaptureCS, L"Raytracing/ReflectionProbeCapture.CS.hlsl");
	CreateComputePipe(Pipe::ReflectionProbeDownsampleCS, L"IBL/ReflectionProbeDownsample.CS.hlsl");
	CreateComputePipe(Pipe::ReflectionProbePrefilterCS, L"IBL/ReflectionProbePrefilter.CS.hlsl");
	CreateComputePipe(Pipe::GodRayCS, L"PostEffect/GodRay.CS.hlsl");
	CreateComputePipe(Pipe::DepthOfFieldCS, L"PostEffect/DepthOfField.CS.hlsl");
	CreateComputePipe(Pipe::MotionBlurCS, L"PostEffect/MotionBlur.CS.hlsl");
	CreateComputePipe(Pipe::LensFlareCS, L"PostEffect/LensFlare.CS.hlsl");
	CreateComputePipe(Pipe::VolumetricLightCompositeCS, L"PostEffect/VolumetricLightComposite.CS.hlsl");
	CreateComputePipe(Pipe::RandomCS, L"PostEffect/Random.CS.hlsl");
	CreatePipe<SkinningCSPipe>(Pipe::SkinningCS);

	CreateComputePipe(Pipe::InitParticleCS, L"Particle/GPU/InitParticle.CS.hlsl");
	CreateComputePipe(Pipe::EmitParticleCS, L"Particle/GPU/EmitParticle.CS.hlsl");
	CreateComputePipe(Pipe::EmitTexParticleCS, L"Particle/GPU/EmitTexParticle.CS.hlsl");
	CreateComputePipe(Pipe::EmitSurfaceParticleCS, L"Particle/GPU/EmitSurfaceParticle.CS.hlsl");
	CreateComputePipe(Pipe::UpdateParticleCS, L"Particle/GPU/UpdateParticle.CS.hlsl");
	CreateComputePipe(Pipe::UpdateParticleSplatCS, L"Particle/GPU/UpdateParticleSplat.CS.hlsl");
	CreateComputePipe(Pipe::ClearParticleColorCS, L"Particle/GPU/ClearParticleColor.CS.hlsl");
	CreatePipe<TrailEmitCSPipe>(Pipe::TrailEmitCS);
	CreateComputePipe(Pipe::InitArgsCS, L"Particle/GPU/InitArgs.CS.hlsl");
	CreateComputePipe(Pipe::AliveCountCS, L"Particle/GPU/AliveParticleCount.CS.hlsl");

	CreateComputePipe(Pipe::SplatClearCS, L"Particle/GPU/SplatClear.CS.hlsl");
	CreateComputePipe(Pipe::SplatParticleCS, L"Particle/GPU/SplatParticle.CS.hlsl");
	// 蓄積バッファを加算合成で重ねるだけなので深度も頂点入力も使わない
	// 頂点入力は無い。深度は見るだけ
	CreateRenderPipe(Pipe::Rain, {
		.vsPath = L"Particle/Rain.VS.hlsl", .psPath = L"Particle/Rain.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::READ_ONLY,
		.useInputLayout = false });
	CreateRenderPipe(Pipe::RainSplash, {
		.vsPath = L"Particle/RainSplash.VS.hlsl", .psPath = L"Particle/Rain.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::READ_ONLY,
		.useInputLayout = false });
	CreateRenderPipe(Pipe::RainDrip, {
		.vsPath = L"Particle/RainDrip.VS.hlsl", .psPath = L"Particle/Rain.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::READ_ONLY,
		.useInputLayout = false });
	CreateRenderPipe(Pipe::RainSpray, {
		.vsPath = L"Particle/RainSpray.VS.hlsl", .psPath = L"Particle/Rain.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::READ_ONLY,
		.useInputLayout = false });
	// 層の距離の深度に置いた画面いっぱいの三角形
	CreateRenderPipe(Pipe::RainCurtain, {
		.vsPath = L"Particle/RainCurtain.VS.hlsl", .psPath = L"Particle/RainCurtain.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::READ_ONLY,
		.useInputLayout = false });
	CreateRenderPipe(Pipe::SplatComposite, {
		.vsPath = L"Particle/GPU/SplatComposite.VS.hlsl", .psPath = L"Particle/GPU/SplatComposite.PS.hlsl",
		.blend = BlendType::ADD_PREMULTIPLIED, .depth = DepthMode::DISABLE,
		.useInputLayout = false, .useDepthTarget = false });

	InitializePending();
}

void PipelineManager::CreateRenderPipe(Pipe type, const RenderPipelineDesc& desc) {
	auto pipe = std::make_unique<RenderPipeline>();
	pipe->SetDesc(desc);
	pipelines_[static_cast<size_t>(type)] = std::move(pipe);
	pendingInit_.push_back(type);
}

void PipelineManager::CreateComputePipe(Pipe type, const std::wstring& csPath) {
	auto pipe = std::make_unique<ComputePipeline>();
	pipe->SetShaderPath(csPath);
	pipelines_[static_cast<size_t>(type)] = std::move(pipe);
	pendingInit_.push_back(type);
}

void PipelineManager::InitializePending() {
	Core::ParallelFor(pendingInit_.size(), [this](size_t i) {
		pipelines_[static_cast<size_t>(pendingInit_[i])]->Initialize(dxcommon_);
	});

	pendingInit_.clear();
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

