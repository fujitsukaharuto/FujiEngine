#include "UpdateParticleCSPipe.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompil.h"
#include "Logger.h"
#include <cassert>


UpdateParticleCSPipe::~UpdateParticleCSPipe() {
}

void UpdateParticleCSPipe::CreateRootSignature(ID3D12Device* device) {
	HRESULT hr;

	CD3DX12_DESCRIPTOR_RANGE descriptorRanges[1] = {};
	descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	rootParameters[1].Descriptor.RegisterSpace = 0;


	// Compute専用なのでフラグは0でOK
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}

	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void UpdateParticleCSPipe::CreatePSO(ID3D12Device* device) {
	HRESULT hr;


	vs = dxcommon_->GetDXCompil()->CompileShader(kDirectoryPath_ + L"CS/Engine/UpdateParticle.CS.hlsl", L"cs_6_0");
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
