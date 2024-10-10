#include"PrimitivePipeline.h"
#include<cassert>
#include"DXCom.h"
#include"DXCompil.h"
#include"Logger.h"

void PrimitivePipeline::CreateRootSignature(ID3D12Device* device){
    HRESULT hr;

    D3D12_ROOT_SIGNATURE_DESC rootDesc {};
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // 定数バッファの設定 (WVP用)
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0;

    rootDesc.pParameters = rootParameters;
    rootDesc.NumParameters = _countof(rootParameters);

    // サンプラの設定は線描画用には不要と仮定
    rootDesc.pStaticSamplers = nullptr;
    rootDesc.NumStaticSamplers = 0;

    // RootSignatureのシリアライズ
    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)){
        if (errorBlob){
            OutputDebugStringA(( char* ) errorBlob->GetBufferPointer());
        }
        assert(false);
    }

    // RootSignatureの作成
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}


void PrimitivePipeline::CreatePSO(ID3D12Device* device){
    HRESULT hr;

    // InputLayoutの設定
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // XYZ
    inputElementDescs[0].AlignedByteOffset = 0;

    inputElementDescs[1].SemanticName = "COLOR";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // RGBA
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC layout {};
    layout.pInputElementDescs = inputElementDescs;
    layout.NumElements = _countof(inputElementDescs);

    // BlendStateの設定
    D3D12_BLEND_DESC blendDesc {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc {};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // カリングを無効化
    rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME; // 線描画の場合はこちら
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthClipEnable = TRUE;

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc {};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // シェーダの読み込み
    vs = DXCom::GetInstance()->GetDXCompil()->CompileShader(kDirectoryPath_ + L"Primitive.VS.hlsl", L"vs_6_0");
    ps = DXCom::GetInstance()->GetDXCompil()->CompileShader(kDirectoryPath_ + L"Primitive.PS.hlsl", L"ps_6_0");
    assert(vs != nullptr);
    assert(ps != nullptr);

    // PSOの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = layout;
    psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    psoDesc.BlendState = blendDesc;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; // 線描画用
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // パイプラインステートオブジェクトの作成
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
    assert(SUCCEEDED(hr));
}
