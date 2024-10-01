#pragma once

#include <wrl/client.h>
#include <dxcapi.h>
#include <string>


using namespace Microsoft::WRL;



class DXCompil {
public:
	DXCompil() = default;
	~DXCompil();

public:

	void Initialize();

	ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);


private:


private:

	ComPtr<IDxcUtils> dxcUtils_;
	ComPtr<IDxcCompiler3> dxcCompiler_;
	ComPtr<IDxcIncludeHandler> includeHandler_;


};
