#include "TrailEmitCSPipe.h"
#include <wrl/client.h>
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Shader/DXCompile.h"
#include "Engine/Logger/Logger.h"
#include "externals/DirectXTex/d3dx12.h"
#include <cassert>

using namespace Graphics;
using namespace DXC;


TrailEmitCSPipe::~TrailEmitCSPipe() {
}

void TrailEmitCSPipe::CreateRootSignature(ID3D12Device* device) {
	HRESULT hr;

	CD3DX12_DESCRIPTOR_RANGE descriptorRanges[9] = {};
	descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
	descriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
	descriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);
	descriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4);
	descriptorRanges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5);
	descriptorRanges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 6);
	descriptorRanges[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 7);
	descriptorRanges[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 8);

	CD3DX12_ROOT_PARAMETER rootParameters[9];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRanges[1];
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRanges[2];
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRanges[3];
	rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[4].DescriptorTable.pDescriptorRanges = &descriptorRanges[4];
	rootParameters[4].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[5].DescriptorTable.pDescriptorRanges = &descriptorRanges[5];
	rootParameters[5].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[6].DescriptorTable.pDescriptorRanges = &descriptorRanges[6];
	rootParameters[6].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[7].DescriptorTable.pDescriptorRanges = &descriptorRanges[7];
	rootParameters[7].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[8].DescriptorTable.pDescriptorRanges = &descriptorRanges[8];
	rootParameters[8].DescriptorTable.NumDescriptorRanges = 1;

	// Compute専用なのでフラグは0でOK
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
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

void TrailEmitCSPipe::CreatePSO(ID3D12Device* device) {
	HRESULT hr;

	vs = dxcommon_->GetDXCompile()->CompileShader(kDirectoryPath_ + L"CS/Engine/TrailEmit.CS.hlsl", L"cs_6_0");
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
