#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <string>

#include "Engine/DXC/Shader/DXCompile.h"
#include <unordered_map>

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// 合成モード
	/// </summary>
	/// <remarks>blendType として JSON に整数で保存される。既存の並び順を変えず、追加は末尾に行うこと</remarks>
	enum class BlendType {
		ALPHA,
		ADD,
		SUBTRACT,
		SCREEN,
		MULTIPLY,
		SOFT_ADD,
		PREMULTIPLIED_ALPHA,
		NONE,				// ブレンドなし(そのまま書き込む)
		ADD_PREMULTIPLIED,	// Src ONE / Dest ONE の加算。乗算済みカラーを重ねる用
	};

	/// <summary>
	/// パイプラインの基底クラス
	/// </summary>
	class BasePipeline {
	public:
		BasePipeline() = default;
		~BasePipeline();

		void Initialize(DXC::DXCom* pDxcom);

		/// <summary>
		/// グラフィックスパイプラインステートを設定
		/// </summary>
		void SetPipelineState();

		/// <summary>同じルートシグネチャのまま、PS を別のマクロでコンパイルした PSO へ切り替える</summary>
		/// <remarks>ルートの結線は消えない。別の PSO を持たないパイプラインでは何もしない</remarks>
		void SetVariantState(bool isVariant);
		bool HasVariant() const { return psoVariant_ != nullptr; }

		/// <summary>SetPipelineState で張る PSO を差し替える。nullptr で Initialize で作ったものへ戻す</summary>
		/// <remarks>ルートシグネチャが同じ PSO であること。マクロ違いの PSO (SetVariantState) は差し替えない</remarks>
		void SetBasePSO(ID3D12PipelineState* pso) { activePso_ = pso ? pso : pso_.Get(); }

		/// <summary>
		/// コンピュートシェーダ用のパイプラインステートを設定
		/// </summary>
		void SetPipelineCSState(uint32_t index = 0);

		/// <summary>
		/// パイプラインを生成する
		/// </summary>
		ID3D12RootSignature* GetRootSignature();

		/// <summary>
		/// ルートパラメータのインデックスを取得
		/// </summary>
		uint32_t GetRootIndex(const std::string& name) const;

	private:

		/// <summary>
		/// ルートシグネチャを生成
		/// </summary>
		virtual void CreateRootSignature(ID3D12Device* device);

		/// <summary>
		/// PSOを生成
		/// </summary>
		virtual void CreatePSO(ID3D12Device* device);

	protected:

		DXC::DXCom* dxcommon_;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_ = nullptr;
		// PS をマクロ違いでコンパイルしたもの。ルートシグネチャは pso_ と同じ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> psoVariant_ = nullptr;
		// SetPipelineState が張るもの。持ち主は pso_ か差し替えた側
		ID3D12PipelineState* activePso_ = nullptr;

		Microsoft::WRL::ComPtr<IDxcBlob> vs = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> ps = nullptr;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> vsReflection_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> psReflection_ = nullptr;

		std::unordered_map<std::string, uint32_t> rootParameterMap_;

		const std::wstring kDirectoryPath_ = L"./resource/Shaders/";

	};

}
