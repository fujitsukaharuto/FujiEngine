#include "SplatCompositePipe.h"
#include "Engine/DX/DXCom.h"
#include "DXCommand.h"
#include "DXCompile.h"
#include "Logger.h"
#include <cassert>

using namespace Graphics;
using namespace DXC;


SplatCompositePipe::~SplatCompositePipe() {}


void SplatCompositePipe::CreateRootSignature(ID3D12Device* device) {
	auto vsData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"SplatComposite.VS.hlsl", L"vs_6_0");
	vs = vsData.blob;
	vsReflection_ = vsData.reflection;
	assert(vs != nullptr);

	auto psData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"SplatComposite.PS.hlsl", L"ps_6_0");
	ps = psData.blob;
	psReflection_ = psData.reflection;
	assert(ps != nullptr);

	rootSignature_ = dxcommon_->GetDXCompile()->CreateRootSignature(device, vsReflection_.Get(), psReflection_.Get(), rootParameterMap_);
	assert(rootSignature_ != nullptr);
}


void SplatCompositePipe::CreatePSO(ID3D12Device* device) {

	HRESULT hr;

	assert(vs != nullptr);
	assert(ps != nullptr);

	// 頂点入力なし(SV_VertexIDでフルスクリーン三角形を生成)
	D3D12_INPUT_LAYOUT_DESC layout{};
	layout.pInputElementDescs = nullptr;
	layout.NumElements = 0;

	// 加算合成: out = sampledColor + dst
	D3D12_BLEND_DESC blend{};
	blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blend.RenderTarget[0].BlendEnable = TRUE;
	blend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	D3D12_RASTERIZER_DESC rasterizer{};
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;
	rasterizer.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable = FALSE;
	depth.StencilEnable = FALSE;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc{};
	stateDesc.pRootSignature = rootSignature_.Get();
	stateDesc.InputLayout = layout;
	stateDesc.VS = { vs->GetBufferPointer(),vs->GetBufferSize() };
	stateDesc.PS = { ps->GetBufferPointer(),ps->GetBufferSize() };
	stateDesc.DepthStencilState = depth;
	stateDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	stateDesc.BlendState = blend;
	stateDesc.RasterizerState = rasterizer;
	stateDesc.NumRenderTargets = 1;
	stateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	stateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	stateDesc.SampleDesc.Count = 1;
	stateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = device->CreateGraphicsPipelineState(&stateDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));
}
