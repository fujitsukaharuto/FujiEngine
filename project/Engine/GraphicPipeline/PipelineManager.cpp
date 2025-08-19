#include "PipelineManager.h"

#include "Engine/DX/DXCom.h"
#include "Pipeline.h"
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
#include "CSPipe/UpdateParticleCSPipe.h"


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
	pipelines_.clear();
	dxcommon_ = nullptr;
}

void PipelineManager::CreatePipeline() {

	//----------------
	// ここどうにかしたい
	//----------------

	std::unique_ptr<Pipeline> pipeline = nullptr;
	std::unique_ptr<Pipeline> pipelineAdd = nullptr;
	std::unique_ptr<SpritePipe> spritePipe = nullptr;
	std::unique_ptr<Line3dPipe> lLine = nullptr;
	std::unique_ptr<ParticlePipeline> particlePipline = nullptr;
	std::unique_ptr<ParticlePipeline> particlePiplineSub = nullptr;
	std::unique_ptr<ParticleCSPipe> particleCSPipe = nullptr;
	std::unique_ptr<AnimationPipeline> animationPipline = nullptr;
	std::unique_ptr<SkyboxPipe> skyboxPipline = nullptr;
	std::unique_ptr<MetaBallPipeline> metaballPipeline = nullptr;
	std::unique_ptr<NonePipeline> nonePipeline = nullptr;
	std::unique_ptr<ShockWavePipe> shockWave = nullptr;
	std::unique_ptr<FirePipe> firePipe = nullptr;
	std::unique_ptr<ThunderPipe> thunderPipe = nullptr;
	std::unique_ptr<CRTPipe> crtPipe = nullptr;
	std::unique_ptr<GrayCSPipe> grayCS = nullptr;
	std::unique_ptr<GaussCSPipe> gaussCS = nullptr;
	std::unique_ptr<BoxFilterCSPipe> boxCS = nullptr;
	std::unique_ptr<RadialBlurCSPipe> radialCS = nullptr;
	std::unique_ptr<VignetteCSPipe> vignetteCS = nullptr;
	std::unique_ptr<CRTCSPipe> crtCS = nullptr;
	std::unique_ptr<RetroTVCSPipe> retroCS = nullptr;
	std::unique_ptr<OutlineCSPipe> outlineCS = nullptr;
	std::unique_ptr<LuminanceOutlineCSPipe> luminanceOutCS = nullptr;
	std::unique_ptr<BloomCSPipe> bloomCS = nullptr;
	std::unique_ptr<RandomCSPipe> randomCS = nullptr;
	std::unique_ptr<SkinningCSPipe> skinningCS = nullptr;
	std::unique_ptr<InitParticleCSPipe> initParticleCS = nullptr;
	std::unique_ptr<EmitterParticleCSPipe> emitParticleCS = nullptr;
	std::unique_ptr<EmitterTexParticleCSPipe> emitTexParticleCS = nullptr;
	std::unique_ptr<UpdateParticleCSPipe> updateParticleCS = nullptr;



	nonePipeline.reset(new NonePipeline());
	nonePipeline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(nonePipeline));


	pipeline.reset(new Pipeline());
	pipeline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(pipeline));


	pipelineAdd.reset(new Pipeline());
	pipelineAdd->SetIsAddMode(true);
	pipelineAdd->Initialize(dxcommon_);
	pipelines_.push_back(std::move(pipelineAdd));


	spritePipe.reset(new SpritePipe());
	spritePipe->Initialize(dxcommon_);
	pipelines_.push_back(std::move(spritePipe));


	lLine.reset(new Line3dPipe());
	lLine->Initialize(dxcommon_);
	pipelines_.push_back(std::move(lLine));


	particlePipline.reset(new ParticlePipeline());
	particlePipline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(particlePipline));


	particlePiplineSub.reset(new ParticlePipeline());
	particlePiplineSub->SetIsSubMode(true);
	particlePiplineSub->Initialize(dxcommon_);
	pipelines_.push_back(std::move(particlePiplineSub));

	particleCSPipe.reset(new ParticleCSPipe());
	particleCSPipe->Initialize(dxcommon_);
	pipelines_.push_back(std::move(particleCSPipe));

	animationPipline.reset(new AnimationPipeline());
	animationPipline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(animationPipline));


	skyboxPipline.reset(new SkyboxPipe());
	skyboxPipline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(skyboxPipline));


	metaballPipeline.reset(new MetaBallPipeline());
	metaballPipeline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(metaballPipeline));


	shockWave.reset(new ShockWavePipe);
	shockWave->Initialize(dxcommon_);
	pipelines_.push_back(std::move(shockWave));

	firePipe.reset(new FirePipe);
	firePipe->Initialize(dxcommon_);
	pipelines_.push_back(std::move(firePipe));


	thunderPipe.reset(new ThunderPipe);
	thunderPipe->Initialize(dxcommon_);
	pipelines_.push_back(std::move(thunderPipe));

	crtPipe.reset(new CRTPipe);
	crtPipe->Initialize(dxcommon_);
	pipelines_.push_back(std::move(crtPipe));

	grayCS.reset(new GrayCSPipe);
	grayCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(grayCS));

	gaussCS.reset(new GaussCSPipe);
	gaussCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(gaussCS));

	boxCS.reset(new BoxFilterCSPipe);
	boxCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(boxCS));

	radialCS.reset(new RadialBlurCSPipe);
	radialCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(radialCS));

	vignetteCS.reset(new VignetteCSPipe);
	vignetteCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(vignetteCS));

	crtCS.reset(new CRTCSPipe);
	crtCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(crtCS));

	retroCS.reset(new RetroTVCSPipe);
	retroCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(retroCS));

	outlineCS.reset(new OutlineCSPipe);
	outlineCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(outlineCS));

	luminanceOutCS.reset(new LuminanceOutlineCSPipe);
	luminanceOutCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(luminanceOutCS));

	bloomCS.reset(new BloomCSPipe);
	bloomCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(bloomCS));

	randomCS.reset(new RandomCSPipe);
	randomCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(randomCS));

	skinningCS.reset(new SkinningCSPipe);
	skinningCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(skinningCS));

	initParticleCS.reset(new InitParticleCSPipe);
	initParticleCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(initParticleCS));

	emitParticleCS.reset(new EmitterParticleCSPipe);
	emitParticleCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(emitParticleCS));

	emitTexParticleCS.reset(new EmitterTexParticleCSPipe);
	emitTexParticleCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(emitTexParticleCS));

	updateParticleCS.reset(new UpdateParticleCSPipe);
	updateParticleCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(updateParticleCS));

}

void PipelineManager::SetPipeline(Pipe type) {
	pipelines_[static_cast<int>(type)]->SetPipelineState();
}

void PipelineManager::SetCSPipeline(Pipe type) {
	pipelines_[static_cast<int>(type)]->SetPipelineCSState();
}
