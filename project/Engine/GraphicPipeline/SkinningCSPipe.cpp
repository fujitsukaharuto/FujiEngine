#include "SkinningCSPipe.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompil.h"
#include "Logger.h"
#include <cassert>


SkinningCSPipe::~SkinningCSPipe() {
}

void SkinningCSPipe::CreateRootSignature(ID3D12Device* device) {
	HRESULT hr;

	CD3DX12_DESCRIPTOR_RANGE rangeSRV[4] = {};
	// t0: matrix palette
	rangeSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	// t1: input vertices
	rangeSRV[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	// t2: vertex influences
	rangeSRV[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	rangeSRV[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);

	CD3DX12_DESCRIPTOR_RANGE rangeUAV;
	// u0: output vertices
	rangeUAV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	D3D12_ROOT_PARAMETER rootParams[7] = {};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[0].DescriptorTable.pDescriptorRanges = &rangeSRV[0];

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].DescriptorTable.pDescriptorRanges = &rangeSRV[1];

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &rangeSRV[2];

	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[3].DescriptorTable.pDescriptorRanges = &rangeUAV;

	rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[4].Descriptor.ShaderRegister = 0; // b0

	rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[5].DescriptorTable.pDescriptorRanges = &rangeSRV[3];

	rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[6].Constants.Num32BitValues = 1;
	rootParams[6].Constants.ShaderRegister = 1;  // b1
	rootParams[6].Constants.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	ComPtr<ID3DBlob> sigBlob;
	ComPtr<ID3DBlob> errBlob;
	hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<const char*>(errBlob->GetBufferPointer()));
		assert(false);
	}

	hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void SkinningCSPipe::CreatePSO(ID3D12Device* device) {
	HRESULT hr;

	vs = dxcommon_->GetDXCompil()->CompileShader(kDirectoryPath_ + L"CS/Engine/Skinning.CS.hlsl", L"cs_6_0");
	assert(vs != nullptr);

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.CS.pShaderBytecode = vs->GetBufferPointer();
	psoDesc.CS.BytecodeLength = vs->GetBufferSize();

	hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));
}
