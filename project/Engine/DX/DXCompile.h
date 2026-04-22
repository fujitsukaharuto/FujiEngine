#pragma once

#include <wrl/client.h>
#include <dxcapi.h>
#include <string>
#include <d3d12shader.h>
#include <d3d12.h>
#include <vector>
#include <unordered_map>


using namespace Microsoft::WRL;

namespace DXC {

	/// <summary>
	/// シェーダーデータ（Blobとリフレクション）
	/// </summary>
	struct ShaderData {
		ComPtr<IDxcBlob> blob;
		ComPtr<ID3D12ShaderReflection> reflection;
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
		ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

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
		/// <returns>ルートシグネチャ</returns>
		ComPtr<ID3D12RootSignature> CreateRootSignature(
			ID3D12Device* device,
			ID3D12ShaderReflection* vsReflection,
			ID3D12ShaderReflection* psReflection,
			std::unordered_map<std::string, uint32_t>& rootParameterMap);

	private:


	private:

		ComPtr<IDxcUtils> dxcUtils_;
		ComPtr<IDxcCompiler3> dxcCompiler_;
		ComPtr<IDxcIncludeHandler> includeHandler_;


	};
}