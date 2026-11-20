#include "Engine/Graphics/Pipeline/Render/RenderPipeline.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Shader/DXCompile.h"
#include "Engine/Logger/Logger.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <format>
#include <cassert>

using namespace Graphics;
using namespace DXC;


RenderPipeline::~RenderPipeline() {}


D3D12_BLEND_DESC RenderPipeline::MakeBlendDesc(BlendType type) {

	// ブレンドなし。そのまま書き込むだけなので既定値をそのまま使う
	if (type == BlendType::NONE) {
		return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	}

	D3D12_BLEND_DESC blend{};
	blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blend.RenderTarget[0].BlendEnable = TRUE;
	blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	switch (type) {
	case BlendType::ALPHA:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendType::ADD:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		break;
	case BlendType::SUBTRACT:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		break;
	case BlendType::SCREEN:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		break;
	case BlendType::MULTIPLY:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		break;
	case BlendType::SOFT_ADD:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		break;
	case BlendType::PREMULTIPLIED_ALPHA:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendType::ADD_PREMULTIPLIED:
		blend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		break;
	default:
		assert(false && "未対応の BlendType");
		break;
	}

	return blend;
}


D3D12_DEPTH_STENCIL_DESC RenderPipeline::MakeDepthStencilDesc(DepthMode mode) {

	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.StencilEnable = FALSE;
	depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	switch (mode) {
	case DepthMode::DISABLE:
		depth.DepthEnable = FALSE;
		depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depth.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		break;
	case DepthMode::READ_WRITE:
		depth.DepthEnable = TRUE;
		depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		break;
	case DepthMode::READ_ONLY:
		depth.DepthEnable = TRUE;
		depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		break;
	default:
		assert(false && "未対応の DepthMode");
		break;
	}

	return depth;
}


void RenderPipeline::CreateRootSignature(ID3D12Device* device) {

	// シェーダーをコンパイルしてリフレクション情報を取得する
	auto vsData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + desc_.vsPath, DXC::kVSProfile);
	vs = vsData.blob;
	vsReflection_ = vsData.reflection;
	assert(vs != nullptr);

	auto psData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + desc_.psPath, DXC::kPSProfile);
	ps = psData.blob;
	psReflection_ = psData.reflection;
	assert(ps != nullptr);

	// マクロ違いの PS。使うリソースはマクロ無しを含むので、ルートシグネチャはこちらのリフレクションから作る
	if (!desc_.psVariantDefine.empty()) {
		auto variantData = dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + desc_.psPath, DXC::kPSProfile,
			desc_.psVariantDefine.c_str());
		psVariant_ = variantData.blob;
		assert(psVariant_ != nullptr);
		psReflection_ = variantData.reflection;
	}

	// リフレクションからルートシグネチャを自動生成（内部でマッピング情報をログ出力）
	rootSignature_ = dxcommon_->GetDXCompile()->CreateRootSignature(device, vsReflection_.Get(), psReflection_.Get(), rootParameterMap_);
	assert(rootSignature_ != nullptr);
}


void RenderPipeline::CreatePSO(ID3D12Device* device) {

	HRESULT hr;

	// すでにコンパイル済みのシェーダーとリフレクションを使用
	assert(vs != nullptr);
	assert(ps != nullptr);
	assert(vsReflection_ != nullptr);
	assert(psReflection_ != nullptr);

	// リフレクションから入力レイアウトを自動生成
	if (desc_.useInputLayout) {
		inputLayout_ = dxcommon_->GetDXCompile()->CreateInputLayout(vsReflection_.Get());
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc = MakeStateDesc(ps.Get());
	hr = device->CreateGraphicsPipelineState(
		&stateDesc, IID_PPV_ARGS(&pso_));
	assert(SUCCEEDED(hr));

	if (psVariant_) {
		stateDesc.PS = { psVariant_->GetBufferPointer(), psVariant_->GetBufferSize() };
		hr = device->CreateGraphicsPipelineState(&stateDesc, IID_PPV_ARGS(&psoVariant_));
		assert(SUCCEEDED(hr));
	}

}

D3D12_GRAPHICS_PIPELINE_STATE_DESC RenderPipeline::MakeStateDesc(IDxcBlob* pixelShader) const {

	D3D12_INPUT_LAYOUT_DESC layout{};
	layout.pInputElementDescs = inputLayout_.empty() ? nullptr : inputLayout_.data();
	layout.NumElements = static_cast<UINT>(inputLayout_.size());

	D3D12_RASTERIZER_DESC rasterizer{};
	rasterizer.CullMode = desc_.cull;
	rasterizer.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc{};
	stateDesc.pRootSignature = rootSignature_.Get();
	stateDesc.InputLayout = layout;
	stateDesc.VS = { vs->GetBufferPointer(),vs->GetBufferSize() };
	stateDesc.PS = { pixelShader->GetBufferPointer(),pixelShader->GetBufferSize() };
	stateDesc.BlendState = MakeBlendDesc(desc_.blend);
	stateDesc.DepthStencilState = MakeDepthStencilDesc(desc_.depth);
	stateDesc.RasterizerState = rasterizer;
	stateDesc.DSVFormat = desc_.useDepthTarget ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;

	stateDesc.NumRenderTargets = 1;
	stateDesc.RTVFormats[0] = desc_.rtvFormat;

	stateDesc.PrimitiveTopologyType = desc_.topology;

	stateDesc.SampleDesc.Count = 1;
	stateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	return stateDesc;
}

ShaderData RenderPipeline::CompilePixelShader(const wchar_t* define) const {
	return dxcommon_->GetDXCompile()->CompileShaderWithReflection(kDirectoryPath_ + desc_.psPath, DXC::kPSProfile, define);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> RenderPipeline::CreatePSOWithPixelShader(IDxcBlob* pixelShader) const {
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	if (pixelShader == nullptr || pixelShader->GetBufferSize() == 0) {
		return pso;
	}
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC stateDesc = MakeStateDesc(pixelShader);
	const HRESULT hr = dxcommon_->GetDevice()->CreateGraphicsPipelineState(&stateDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		Logger::Log(std::format("CreatePSOWithPixelShader failed: 0x{:08X}", static_cast<uint32_t>(hr)));
		pso.Reset();
	}
	return pso;
}
