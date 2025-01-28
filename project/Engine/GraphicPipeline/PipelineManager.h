#pragma once
#include "BasePipeline.h"
#include <memory>
#include <vector>


enum class Pipe : int {

	None,
	Normal,
	Line3d,
	particle,
	particleSab,
	Gray,
	Gauss,
	MetaBall,
	ShockWave,
	Fire,
	Thunder,
	CRT,
	Plane,
};


class PipelineManager {
public:
	PipelineManager() = default;
	~PipelineManager();

public:

	static PipelineManager* GetInstance();

	void Initialize();

	void Finalize();

	void CreatePipeline();



	void SetPipeline(Pipe type);


private:


private:


	std::vector<std::unique_ptr<BasePipeline>> pipelines_;


};