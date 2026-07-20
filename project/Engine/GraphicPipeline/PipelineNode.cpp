#include "Engine/GraphicPipeline/PipelineNode.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DXCommand.h"
#include "Engine/DX/DXCompile.h"
#include "Engine/Logger/Logger.h"
#include <cassert>

using namespace Graphics;
using namespace DXC;



PipelineNode::~PipelineNode() {}


void PipelineNode::CreateRootSignature(ID3D12Device* device) {

	// シェーダーをコンパイルしてリフレクション情報を取得する
	auto vsData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"Object3d.VS.hlsl", L"vs_6_0");
	vs = vsData.blob;
	vsReflection_ = vsData.reflection;
	assert(vs != nullptr);

	auto psData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + L"Object3dNode.PS.hlsl", L"ps_6_0");
	ps = psData.blob;
	psReflection_ = psData.reflection;
	assert(ps != nullptr);

	rootSignature_ = dxcommon_->GetDXCompile()->CreateRootSignature(device, vsReflection_.Get(), psReflection_.Get(), rootParameterMap_);
	assert(rootSignature_ != nullptr);
}


void PipelineNode::CreatePSO(ID3D12Device* device) {

	HRESULT hr;

	D3D12_BLEND_DESC blend{};
	if (!isAddMode_) {
		blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blend.RenderTarget[0].BlendEnable = TRUE;
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	} else {
		blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blend.RenderTarget[0].BlendEnable = TRUE;
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	}

	// すでにコンパイル済みのシェーダーとリフレクションを使用
	assert(vs != nullptr);
	assert(ps != nullptr);
	assert(vsReflection_ != nullptr);
	assert(psReflection_ != nullptr);

	// リフレクションから入力レイアウトを自動生成
	auto inputLayout = dxcommon_->GetDXCompile()->CreateInputLayout(vsReflection_.Get());

	D3D12_RASTERIZER_DESC rasterizer{};
	rasterizer.CullMode = D3D12_CULL_MODE_NONE;
	rasterizer.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_INPUT_LAYOUT_DESC layout{};
	layout.pInputElementDescs = inputLayout.data();
	layout.NumElements = static_cast<UINT>(inputLayout.size());

	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable = true;
	if (!isAddMode_) {
		depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	} else {
		depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc{};
	stateDesc.pRootSignature = rootSignature_.Get();
	stateDesc.InputLayout = layout;
	stateDesc.VS = { vs->GetBufferPointer(),vs->GetBufferSize() };
	stateDesc.PS = { ps->GetBufferPointer(),ps->GetBufferSize() };
	stateDesc.DepthStencilState = depth;
	stateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	stateDesc.BlendState = blend;
	stateDesc.RasterizerState = rasterizer;

	stateDesc.NumRenderTargets = 1;
	stateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	stateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	stateDesc.SampleDesc.Count = 1;
	stateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = device->CreateGraphicsPipelineState(
		&stateDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));

}
