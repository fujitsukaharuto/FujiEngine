#include "UpdateParticleSplatCSPipe.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DXCommand.h"
#include "Engine/DX/DXCompile.h"
#include "Engine/Logger/Logger.h"
#include <cassert>

using namespace Graphics;
using namespace DXC;


UpdateParticleSplatCSPipe::~UpdateParticleSplatCSPipe() {
}

void UpdateParticleSplatCSPipe::CreateRootSignature(ID3D12Device* device) {
	auto csData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"CS/Engine/UpdateParticleSplat.CS.hlsl", L"cs_6_0");

	vs = csData.blob;
	vsReflection_ = csData.reflection;
	assert(vs != nullptr);

	rootSignature_ = dxcommon_->GetDXCompile()->CreateRootSignature(device, vsReflection_.Get(), rootParameterMap_);
	assert(rootSignature_ != nullptr);
}

void UpdateParticleSplatCSPipe::CreatePSO(ID3D12Device* device) {
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
