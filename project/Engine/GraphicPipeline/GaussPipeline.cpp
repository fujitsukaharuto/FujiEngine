#include "GaussPipeline.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompil.h"
#include "Logger.h"
#include <cassert>



GaussPipeline::~GaussPipeline() {}


void GaussPipeline::CreateRootSignature(ID3D12Device* device) {

	HRESULT hr;

	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	CD3DX12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	D3D12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootDesc.pParameters = rootParameters;
	rootDesc.NumParameters = _countof(rootParameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootDesc.pStaticSamplers = staticSamplers;
	rootDesc.NumStaticSamplers = _countof(staticSamplers);


	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

}


void GaussPipeline::CreatePSO(ID3D12Device* device) {

	HRESULT hr;

	D3D12_INPUT_ELEMENT_DESC element[2] = {};
	element[0].SemanticName = "POSITION";
	element[0].SemanticIndex = 0;
	element[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	element[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	element[1].SemanticName = "TEXCOORD";
	element[1].SemanticIndex = 0;
	element[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	element[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC layout{};
	layout.pInputElementDescs = element;
	layout.NumElements = _countof(element);


	vs = dxcommon_->GetDXCompil()->CompileShader(kDirectoryPath_ + L"NonePost.VS.hlsl", L"vs_6_0");
	assert(vs != nullptr);
	ps = dxcommon_->GetDXCompil()->CompileShader(kDirectoryPath_ + L"GaussianBlur.PS.hlsl", L"ps_6_0");
	assert(ps != nullptr);


	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable = FALSE;
	depth.StencilEnable = FALSE;
	depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc{};
	stateDesc.pRootSignature = rootSignature_.Get();
	stateDesc.InputLayout = layout;
	stateDesc.VS = { vs->GetBufferPointer(),vs->GetBufferSize() };
	stateDesc.PS = { ps->GetBufferPointer(),ps->GetBufferSize() };
	stateDesc.DepthStencilState = depth;
	stateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	stateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	stateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	stateDesc.NumRenderTargets = 1;
	stateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	stateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	stateDesc.SampleDesc.Count = 1;
	stateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = device->CreateGraphicsPipelineState(
		&stateDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));

}
