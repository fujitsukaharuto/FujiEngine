#include "InitArgsCSPipe.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompile.h"
#include "Logger.h"
#include <cassert>

using namespace Graphics;
using namespace DXC;


InitArgsCSPipe::~InitArgsCSPipe() {
}

void InitArgsCSPipe::CreateRootSignature(ID3D12Device* device) {
	auto csData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"CS/Engine/InitArgs.CS.hlsl", L"cs_6_0");

	vs = csData.blob;
	vsReflection_ = csData.reflection;
	assert(vs != nullptr);

	rootSignature_ = dxcommon_->GetDXCompile()->CreateRootSignature(device, vsReflection_.Get(), rootParameterMap_);
	assert(rootSignature_ != nullptr);
}

void InitArgsCSPipe::CreatePSO(ID3D12Device* device) {
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
