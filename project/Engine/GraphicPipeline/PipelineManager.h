#pragma once
#include "BasePipeline.h"
#include "PipeKind.h"
#include <memory>
#include <vector>


class DXCom;

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