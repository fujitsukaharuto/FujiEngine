#include "BasePipeline.h"
#include "DXCom.h"
#include "DXCommand.h"


BasePipeline::~BasePipeline() {
	pso_.Reset();
	rootSignature_.Reset();
	ps.Reset();
	vs.Reset();
}

void BasePipeline::Initialize() {

	CreateRootSignature(DXCom::GetInstance()->GetDevice());
	CreatePSO(DXCom::GetInstance()->GetDevice());

}

void BasePipeline::SetPipelineState() {

	ID3D12GraphicsCommandList* commandList = DXCom::GetInstance()->GetDXCommand()->GetList();
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pso_.Get());

}

void BasePipeline::SetPipelineCSState() {
	ID3D12GraphicsCommandList* commandList = DXCom::GetInstance()->GetDXCommand()->GetList();
	commandList->SetComputeRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pso_.Get());
}

void BasePipeline::CreateRootSignature([[maybe_unused]] ID3D12Device* device) {
}

void BasePipeline::CreatePSO([[maybe_unused]] ID3D12Device* device) {
}
