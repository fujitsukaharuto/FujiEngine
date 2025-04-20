#include "BasePipeline.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"


BasePipeline::~BasePipeline() {
	pso_.Reset();
	rootSignature_.Reset();
	ps.Reset();
	vs.Reset();
	dxcommon_ = nullptr;
}

void BasePipeline::Initialize(DXCom* pDxcom) {

	dxcommon_ = pDxcom;

	CreateRootSignature(dxcommon_->GetDevice());
	CreatePSO(dxcommon_->GetDevice());

}

void BasePipeline::SetPipelineState() {

	ID3D12GraphicsCommandList* commandList = dxcommon_->GetDXCommand()->GetList();
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pso_.Get());

}

void BasePipeline::SetPipelineCSState() {
	ID3D12GraphicsCommandList* commandList = dxcommon_->GetDXCommand()->GetList();
	commandList->SetComputeRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pso_.Get());
}

void BasePipeline::CreateRootSignature([[maybe_unused]] ID3D12Device* device) {
}

void BasePipeline::CreatePSO([[maybe_unused]] ID3D12Device* device) {
}
