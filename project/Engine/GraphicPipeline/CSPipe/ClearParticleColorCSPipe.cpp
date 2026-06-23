#include "ClearParticleColorCSPipe.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompile.h"
#include "Logger.h"
#include <cassert>


ClearParticleColorCSPipe::~ClearParticleColorCSPipe() {
}

void ClearParticleColorCSPipe::CreateRootSignature(ID3D12Device* device) {
	auto csData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"CS/Engine/ClearParticleColor.CS.hlsl", L"cs_6_0");

	vs = csData.blob;
	vsReflection_ = csData.reflection;
	assert(vs != nullptr);

	rootSignature_ = dxcommon_->GetDXCompile()->CreateRootSignature(device, vsReflection_.Get(), rootParameterMap_);
	assert(rootSignature_ != nullptr);
}

void ClearParticleColorCSPipe::CreatePSO(ID3D12Device* device) {
	HRESULT hr;

	assert(vs != nullptr);

	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {};
	computePipelineStateDesc.pRootSignature = rootSignature_.Get();
	computePipelineStateDesc.CS.pShaderBytecode = vs->GetBufferPointer();
	computePipelineStateDesc.CS.BytecodeLength = vs->GetBufferSize();
	computePipelineStateDesc.NodeMask = 0;
	computePipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	hr = device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));
}
