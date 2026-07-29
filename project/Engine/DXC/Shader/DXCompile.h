#pragma once

#include <wrl/client.h>
#include <dxcapi.h>
#include <string>
#include <d3d12shader.h>
#include <d3d12.h>
#include <vector>
#include <unordered_map>


namespace DXC {

	/// <summary>
	/// コンパイルに使うシェーダーモデル
	/// </summary>
	/// <remarks>
	/// インラインレイトレ（RayQuery）が 6.5 以上を要求するため、段階ごとに混ざらないよう全体を 6.5 で揃えている。
	/// 上げ下げはここ3行だけで済むが、実行環境も 6.5 を満たしている必要がある
	/// （GraphicsDevice::GetHighestShaderModel が起動ログに出る）。
	/// </remarks>
	constexpr const wchar_t* kVSProfile = L"vs_6_5";
	constexpr const wchar_t* kPSProfile = L"ps_6_5";
	constexpr const wchar_t* kCSProfile = L"cs_6_5";

	/// <summary>
	/// シェーダーデータ（Blobとリフレクション）
	/// </summary>
	struct ShaderData {
		Microsoft::WRL::ComPtr<IDxcBlob> blob;
		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
	};

	/// <summary>
	/// シェーダーのリソースバインド情報
	/// </summary>
	struct ShaderResourceBinding {
		std::string name;
		uint32_t bindPoint;
		uint32_t bindCount;
		uint32_t space;
		D3D_SHADER_INPUT_TYPE type;
	};

	/// <summary>
	/// コンパイラー関係
	/// </summary>
	class DXCompile {
	public:
		DXCompile() = default;
		~DXCompile();

	public:

		void Initialize();

		/// <summary>
		/// シェーダーのコンパイル関数
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <param name="profile">シェーダーレベル</param>
		/// <returns>IDxcBlob*</returns>
		Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

		/// <summary>
		/// シェーダーのコンパイルとリフレクション取得
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <param name="profile">シェーダーレベル</param>
		/// <returns>ShaderData</returns>
		ShaderData CompileShaderWithReflection(const std::wstring& filePath, const wchar_t* profile);

		/// <summary>
		/// リフレクションから入力レイアウトを生成
		/// </summary>
		/// <param name="reflection">リフレクションデータ</param>
		/// <returns>入力レイアウトの配列</returns>
		std::vector<D3D12_INPUT_ELEMENT_DESC> CreateInputLayout(ID3D12ShaderReflection* reflection);

		/// <summary>
		/// リソースバインド情報を取得
		/// </summary>
		/// <param name="reflection">リフレクションデータ</param>
		/// <returns>リソース情報のリスト</returns>
		std::vector<ShaderResourceBinding> ReflectResources(ID3D12ShaderReflection* reflection);

		/// <summary>
		/// リフレクションからルートシグネチャを生成
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="vsReflection">VSのリフレクション</param>
		/// <param name="psReflection">PSのリフレクション</param>
		/// <param name="rootParameterMap">ルートパラメータのインデックスマップ</param>
		/// <param name="vsVisibility">VSのVisibility (デフォルト VERTEX)</param>
		/// <param name="psVisibility">PSのVisibility (デフォルト PIXEL)</param>
		/// <returns>ルートシグネチャ</returns>
		Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(
			ID3D12Device* device,
			ID3D12ShaderReflection* vsReflection,
			ID3D12ShaderReflection* psReflection,
			std::unordered_map<std::string, uint32_t>& rootParameterMap,
			D3D12_SHADER_VISIBILITY vsVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
			D3D12_SHADER_VISIBILITY psVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
			const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers = {});

		/// <summary>
		/// リフレクションからルートシグネチャを生成 (CS用)
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="csReflection">CSのリフレクション</param>
		/// <param name="rootParameterMap">ルートパラメータのインデックスマップ</param>
		/// <returns>ルートシグネチャ</returns>
		Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(
			ID3D12Device* device,
			ID3D12ShaderReflection* csReflection,
			std::unordered_map<std::string, uint32_t>& rootParameterMap,
			const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers = {});

	private:


	private:

		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
		Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;


	};
}