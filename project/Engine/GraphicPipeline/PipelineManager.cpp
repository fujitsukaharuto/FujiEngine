#include "PipelineManager.h"

#include "Engine/DX/DXCom.h"
#include "Pipeline.h"
#include "Line3dPipe.h"
#include "ParticlePipeline.h"
#include "AnimationPipeline.h"
#include "SkyboxPipe.h"
#include "GrayPipeline.h"
#include "MetaBallPipeline.h"
#include "GaussPipeline.h"
#include "NonePipeline.h"
#include "ShockWavePipe.h"
#include "FirePipe.h"
#include "ThunderPipe.h"
#include "CRTPipe.h"
#include "GrayCSPipe.h"
#include "CRTCSPipe.h"
#include "SkinningCSPipe.h"


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


	std::unique_ptr<Pipeline> pipeline = nullptr;
	std::unique_ptr<Pipeline> pipelineAdd = nullptr;
	std::unique_ptr<Line3dPipe> lLine = nullptr;
	std::unique_ptr<ParticlePipeline> particlePipline = nullptr;
	std::unique_ptr<ParticlePipeline> particlePiplineSub = nullptr;
	std::unique_ptr<AnimationPipeline> animationPipline = nullptr;
	std::unique_ptr<SkyboxPipe> skyboxPipline = nullptr;
	std::unique_ptr<GrayPipeline> grayPipeline = nullptr;
	std::unique_ptr<MetaBallPipeline> metaballPipeline = nullptr;
	std::unique_ptr<GaussPipeline> gaussPipeline = nullptr;
	std::unique_ptr<NonePipeline> nonePipeline = nullptr;
	std::unique_ptr<ShockWavePipe> shockWave = nullptr;
	std::unique_ptr<FirePipe> firePipe = nullptr;
	std::unique_ptr<ThunderPipe> thunderPipe = nullptr;
	std::unique_ptr<CRTPipe> crtPipe = nullptr;
	std::unique_ptr<GrayCSPipe> grayCS = nullptr;
	std::unique_ptr<CRTCSPipe> crtCS = nullptr;
	std::unique_ptr<SkinningCSPipe> skinningCS = nullptr;



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


	animationPipline.reset(new AnimationPipeline());
	animationPipline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(animationPipline));


	skyboxPipline.reset(new SkyboxPipe());
	skyboxPipline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(skyboxPipline));


	grayPipeline.reset(new GrayPipeline());
	grayPipeline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(grayPipeline));


	gaussPipeline.reset(new GaussPipeline());
	gaussPipeline->Initialize(dxcommon_);
	pipelines_.push_back(std::move(gaussPipeline));


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

	crtCS.reset(new CRTCSPipe);
	crtCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(crtCS));

	skinningCS.reset(new SkinningCSPipe);
	skinningCS->Initialize(dxcommon_);
	pipelines_.push_back(std::move(skinningCS));

}

void PipelineManager::SetPipeline(Pipe type) {
	pipelines_[static_cast<int>(type)]->SetPipelineState();
}

void PipelineManager::SetCSPipeline(Pipe type) {
	pipelines_[static_cast<int>(type)]->SetPipelineCSState();
}
