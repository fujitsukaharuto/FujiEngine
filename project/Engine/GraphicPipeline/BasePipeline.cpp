#include "Engine/GraphicPipeline/BasePipeline.h"
#include <string>
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DXCommand.h"
#include "Engine/Logger/Logger.h"
#include <cassert>

using namespace Graphics;
using namespace DXC;


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

void BasePipeline::SetPipelineCSState(uint32_t index) {
	if (index == 0) {
		ID3D12GraphicsCommandList* commandList = dxcommon_->GetDXCommand()->GetList();
		commandList->SetComputeRootSignature(rootSignature_.Get());
		commandList->SetPipelineState(pso_.Get());
	} else if (index == 1) {
		ID3D12GraphicsCommandList* commandList = dxcommon_->GetDXCommand()->GetImmediateList();
		commandList->SetComputeRootSignature(rootSignature_.Get());
		commandList->SetPipelineState(pso_.Get());
	} else {
		ID3D12GraphicsCommandList* commandList = dxcommon_->GetDXCommand()->GetComputeList();
		commandList->SetComputeRootSignature(rootSignature_.Get());
		commandList->SetPipelineState(pso_.Get());
	}
}

ID3D12RootSignature* BasePipeline::GetRootSignature() {
	return rootSignature_.Get();
}

uint32_t BasePipeline::GetRootIndex(const std::string& name) const {
	auto it = rootParameterMap_.find(name);
	if (it != rootParameterMap_.end()) {
		return it->second;
	}
	// 見つからない = シェーダ側の宣言と名前が食い違っている。0 を返すと別のリソースを
	// 黙ってバインドし続けるので、開発中は必ず止める
	Logger::Log(std::format("Warning: RootParameter '{}' not found in current pipeline reflection map.", name));
	assert(false && "RootParameter not found. RootNames.h の綴りが HLSL 側と一致しているか確認すること");
	return 0;
}

void BasePipeline::CreateRootSignature([[maybe_unused]] ID3D12Device* device) {
}

void BasePipeline::CreatePSO([[maybe_unused]] ID3D12Device* device) {
}
