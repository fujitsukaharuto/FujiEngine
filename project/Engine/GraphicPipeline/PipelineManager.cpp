#include "PipelineManager.h"

#include "Pipeline.h"
#include "Line3dPipe.h"
#include "ParticlePipeline.h"
#include "ParticleSabPipe.h"
#include "GrayPipeline.h"
#include "MetaBallPipeline.h"
#include "GaussPipeline.h"
#include "NonePipeline.h"
#include "ShockWavePipe.h"
#include "FirePipe.h"
#include "ThunderPipe.h"
#include "CRTPipe.h"
#include "PlaneDrawerPipe.h"


PipelineManager::~PipelineManager() {
	pipelines_.clear();
}

PipelineManager* PipelineManager::GetInstance() {
	static PipelineManager instance;
	return &instance;
}

void PipelineManager::Initialize() {
}

void PipelineManager::Finalize() {
}

void PipelineManager::CreatePipeline() {


	std::unique_ptr<Pipeline> pipeline = nullptr;
	std::unique_ptr<Line3dPipe> lLine = nullptr;
	std::unique_ptr<ParticlePipeline> particlePipline = nullptr;
	std::unique_ptr<ParticleSabPipe> particleSabPipline = nullptr;
	std::unique_ptr<GrayPipeline> grayPipeline = nullptr;
	std::unique_ptr<MetaBallPipeline> metaballPipeline = nullptr;
	std::unique_ptr<GaussPipeline> gaussPipeline = nullptr;
	std::unique_ptr<NonePipeline> nonePipeline = nullptr;
	std::unique_ptr<ShockWavePipe> shockWave = nullptr;
	std::unique_ptr<FirePipe> firePipe = nullptr;
	std::unique_ptr<ThunderPipe> thunderPipe = nullptr;
	std::unique_ptr<CRTPipe> crtPipe = nullptr;
	std::unique_ptr<PlanePipe> plane = nullptr;


	nonePipeline.reset(new NonePipeline());
	nonePipeline->Initialize();
	pipelines_.push_back(std::move(nonePipeline));


	pipeline.reset(new Pipeline());
	pipeline->Initialize();
	pipelines_.push_back(std::move(pipeline));


	lLine.reset(new Line3dPipe());
	lLine->Initialize();
	pipelines_.push_back(std::move(lLine));


	particlePipline.reset(new ParticlePipeline());
	particlePipline->Initialize();
	pipelines_.push_back(std::move(particlePipline));


	particleSabPipline.reset(new ParticleSabPipe());
	particleSabPipline->Initialize();
	pipelines_.push_back(std::move(particleSabPipline));


	grayPipeline.reset(new GrayPipeline());
	grayPipeline->Initialize();
	pipelines_.push_back(std::move(grayPipeline));


	gaussPipeline.reset(new GaussPipeline());
	gaussPipeline->Initialize();
	pipelines_.push_back(std::move(gaussPipeline));


	metaballPipeline.reset(new MetaBallPipeline());
	metaballPipeline->Initialize();
	pipelines_.push_back(std::move(metaballPipeline));


	shockWave.reset(new ShockWavePipe);
	shockWave->Initialize();
	pipelines_.push_back(std::move(shockWave));

	firePipe.reset(new FirePipe);
	firePipe->Initialize();
	pipelines_.push_back(std::move(firePipe));


	thunderPipe.reset(new ThunderPipe);
	thunderPipe->Initialize();
	pipelines_.push_back(std::move(thunderPipe));

	crtPipe.reset(new CRTPipe);
	crtPipe->Initialize();
	pipelines_.push_back(std::move(crtPipe));

	plane.reset(new PlanePipe);
	plane->Initialize();
	pipelines_.push_back(std::move(plane));

}

void PipelineManager::SetPipeline(Pipe type) {
	pipelines_[static_cast<int>(type)]->SetPipelineState();
}