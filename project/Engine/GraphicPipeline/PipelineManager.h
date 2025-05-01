#pragma once
#include "BasePipeline.h"
#include <memory>
#include <vector>


class DXCom;

enum class Pipe : int {

	None,
	Normal,
	Line3d,
	particle,
	particleSub,
	Animation,
	Gray,
	Gauss,
	MetaBall,
	ShockWave,
	Fire,
	Thunder,
	CRT,
	GrayCS,
	CRTCS,
};


class PipelineManager {
public:
	PipelineManager() = default;
	~PipelineManager();

public:

	static PipelineManager* GetInstance();

	void Initialize(DXCom* pDxcom);

	void Finalize();

	void CreatePipeline();



	void SetPipeline(Pipe type);
	void SetCSPipeline(Pipe type);


private:


private:

	DXCom* dxcommon_;
	std::vector<std::unique_ptr<BasePipeline>> pipelines_;


};