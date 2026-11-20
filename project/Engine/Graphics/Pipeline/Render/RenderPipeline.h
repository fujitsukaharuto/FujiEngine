#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include "Engine/Graphics/Pipeline/BasePipeline.h"
#include "Engine/DXC/Resource/DX12Helper.h"


namespace Graphics {

	/// <summary>
	/// 深度ステンシルの使い方
	/// </summary>
	enum class DepthMode {
		DISABLE,		// 深度テストなし(フルスクリーンパス・2D)
		READ_WRITE,		// テストして書き込む(不透明)
		READ_ONLY,		// テストするが書き込まない(半透明・加算)
	};

	/// <summary>
	/// 描画パイプラインの構成。パイプ毎の差分はこの構造体だけで表現する
	/// </summary>
	struct RenderPipelineDesc {
		std::wstring vsPath;	// kDirectoryPath_ からの相対
		std::wstring psPath;	// kDirectoryPath_ からの相対
		// 空でなければ、PS をこのマクロ付きでもう1つコンパイルし、同じルートシグネチャの PSO を作る (SetVariantState で切り替え)
		std::wstring psVariantDefine;
		// true なら PS を OBJECT_FEATURES 違いでも作れる (PipelineManager::SetObjectFeatures)
		bool hasObjectFeatures = false;
		BlendType blend = BlendType::ALPHA;
		DepthMode depth = DepthMode::READ_WRITE;
		D3D12_CULL_MODE cull = D3D12_CULL_MODE_NONE;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		bool useInputLayout = true;		// false = 頂点入力なし(SV_VertexID からフルスクリーン三角形を作る類)
		bool useDepthTarget = true;		// false = DSVFormat を UNKNOWN にする(深度バッファを一切束ねないパス)

		// 書き込み先のフォーマット。束ねる RTV と一致していないと PSO 作成時に弾かれる。
		// ほとんどの描画はオフスクリーン宛なので既定はそちら。画面へ直接出す最終パスだけ上書きする
		DXGI_FORMAT rtvFormat = DXC::kSceneColorFormat;
	};

	/// <summary>
	/// リフレクション駆動でルートシグネチャを生成する標準的な描画パイプライン
	/// </summary>
	/// <remarks>差分が RenderPipelineDesc で表現できるものは全てここに集約する</remarks>
	class RenderPipeline : public BasePipeline {
	public:
		RenderPipeline() = default;
		~RenderPipeline();

		/// <summary>構成を Initialize より前に設定する</summary>
		void SetDesc(const RenderPipelineDesc& desc) { desc_ = desc; }
		const RenderPipelineDesc& GetDesc() const { return desc_; }

		/// <summary>PS をマクロ付きでコンパイルする。Initialize の後なら別スレッドから呼んでよい</summary>
		/// <param name="define">-D で渡すマクロ (例 L"OBJECT_FEATURES=0x3")</param>
		DXC::ShaderData CompilePixelShader(const wchar_t* define) const;

		/// <summary>PS だけを差し替えた PSO を作る。Initialize の後なら別スレッドから呼んでよい</summary>
		/// <returns>失敗したら nullptr</returns>
		Microsoft::WRL::ComPtr<ID3D12PipelineState> CreatePSOWithPixelShader(IDxcBlob* pixelShader) const;

		/// <summary>
		/// 合成モードから D3D12_BLEND_DESC を作る
		/// </summary>
		static D3D12_BLEND_DESC MakeBlendDesc(BlendType type);

		/// <summary>
		/// 深度の使い方から D3D12_DEPTH_STENCIL_DESC を作る
		/// </summary>
		static D3D12_DEPTH_STENCIL_DESC MakeDepthStencilDesc(DepthMode mode);

	private:

		void CreateRootSignature(ID3D12Device* device)override;

		void CreatePSO(ID3D12Device* device)override;

		/// <summary>PS 以外を desc_ から埋めた PSO の構成。入力レイアウトは inputLayout_ を指す</summary>
		D3D12_GRAPHICS_PIPELINE_STATE_DESC MakeStateDesc(IDxcBlob* pixelShader) const;

	private:

		RenderPipelineDesc desc_;
		Microsoft::WRL::ComPtr<IDxcBlob> psVariant_ = nullptr;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;

	};

}
