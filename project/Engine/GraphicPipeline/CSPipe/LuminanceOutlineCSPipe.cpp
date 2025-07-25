#include "LuminanceOutlineCSPipe.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompil.h"
#include "Logger.h"
#include <cassert>


LuminanceOutlineCSPipe::~LuminanceOutlineCSPipe() {
}

void LuminanceOutlineCSPipe::CreateRootSignature(ID3D12Device* device) {
    HRESULT hr;

    // コンピュートシェーダー用のルートシグネチャ
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    CD3DX12_DESCRIPTOR_RANGE descriptorRangeSRV[2] = {};
    descriptorRangeSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    descriptorRangeSRV[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    // パラメータは3つ。SRV(入力テクスチャ)、UAV(出力テクスチャ)、定数バッファ
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRangeSRV[0];
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRangeSRV[1];
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);

    // サンプラーの設定
    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.NumStaticSamplers = 1;

    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

void LuminanceOutlineCSPipe::CreatePSO(ID3D12Device* device) {
    HRESULT hr;


    vs = dxcommon_->GetDXCompil()->CompileShader(kDirectoryPath_ + L"CS/LuminanceBasedOutline.CS.hlsl", L"cs_6_0");
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
