#pragma once

#include <wrl/client.h>
#include <dxcapi.h>
#include <string>


using namespace Microsoft::WRL;

namespace DXC {
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

	private:


	private:

		ComPtr<IDxcUtils> dxcUtils_;
		ComPtr<IDxcCompiler3> dxcCompiler_;
		ComPtr<IDxcIncludeHandler> includeHandler_;


	};
}