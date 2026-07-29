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
	/// <remarks>
	/// パーティクルグループの blendType として JSON に整数で保存され、DebugGUI のコンボも
	/// この並び順を index として引いている。既存の値の順番を変えず、追加は必ず末尾に行うこと
	/// </remarks>
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

		Microsoft::WRL::ComPtr<IDxcBlob> vs = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> ps = nullptr;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> vsReflection_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> psReflection_ = nullptr;

		std::unordered_map<std::string, uint32_t> rootParameterMap_;

		const std::wstring kDirectoryPath_ = L"./resource/Shaders/";

	};

}
