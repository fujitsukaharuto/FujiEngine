#include "DXCompile.h"
#include <iostream>
#include "Logger.h"
#include <cassert>

#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"dxguid.lib")

using namespace DXC;


DXCompile::~DXCompile() {
	dxcUtils_.Reset();
	dxcCompiler_.Reset();
	includeHandler_.Reset();
}


void DXCompile::Initialize() {

	HRESULT hr;

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));

}

ComPtr<IDxcBlob> DXCompile::CompileShader(const std::wstring& filePath, const wchar_t* profile) {
	return CompileShaderWithReflection(filePath, profile).blob;
}

ShaderData DXCompile::CompileShaderWithReflection(const std::wstring& filePath, const wchar_t* profile) {

	Logger::Log((std::format(L"Begin CompileShader, path:{},profile:{}", filePath, profile)));
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

#ifdef _DEBUG
	// argumentの設定
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};
#else
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-O3",
		L"-Zpr",
	};
#endif

	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler_.Get(),
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));


	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Logger::Log(shaderError->GetStringPointer()); // Logを出せるようにする
		assert(false);
	}


	ShaderData result;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&result.blob), nullptr);
	assert(SUCCEEDED(hr));

	// リフレクションの取得
	IDxcBlob* reflectionBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);
	if (SUCCEEDED(hr) && reflectionBlob != nullptr) {
		DxcBuffer reflectionBuffer;
		reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
		reflectionBuffer.Size = reflectionBlob->GetBufferSize();
		reflectionBuffer.Encoding = 0;
		dxcUtils_->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&result.reflection));
	}

	Logger::Log((std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));// 成功Logを出せるようにする
	shaderSource->Release();
	shaderResult->Release();
	return result;

}

std::vector<D3D12_INPUT_ELEMENT_DESC> DXCompile::CreateInputLayout(ID3D12ShaderReflection* reflection) {
	assert(reflection != nullptr);

	D3D12_SHADER_DESC shaderDesc;
	reflection->GetDesc(&shaderDesc);

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
	for (uint32_t i = 0; i < shaderDesc.InputParameters; ++i) {
		D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
		reflection->GetInputParameterDesc(i, &paramDesc);

		D3D12_INPUT_ELEMENT_DESC elementDesc{};
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;

		// 特定のセマンティック（WEIGHT, INDEX）の場合は Slot 1 を使用する（エンジン規定）
		std::string semanticName = paramDesc.SemanticName;
		if (semanticName == "WEIGHT" || semanticName == "INDEX") {
			elementDesc.InputSlot = 1;
		}

		elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// フォーマットの判定
		if (paramDesc.Mask == 1) { // 1成分
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		} else if (paramDesc.Mask <= 3) { // 2成分
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		} else if (paramDesc.Mask <= 7) { // 3成分
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		} else if (paramDesc.Mask <= 15) { // 4成分
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		inputLayout.push_back(elementDesc);
	}

	return inputLayout;
}

std::vector<ShaderResourceBinding> DXCompile::ReflectResources(ID3D12ShaderReflection* reflection) {
	std::vector<ShaderResourceBinding> bindings;
	if (!reflection) return bindings;

	D3D12_SHADER_DESC shaderDesc;
	reflection->GetDesc(&shaderDesc);

	for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
		D3D12_SHADER_INPUT_BIND_DESC bindDesc;
		reflection->GetResourceBindingDesc(i, &bindDesc);

		ShaderResourceBinding binding;
		binding.name = bindDesc.Name;
		binding.bindPoint = bindDesc.BindPoint;
		binding.bindCount = bindDesc.BindCount;
		binding.space = bindDesc.Space;
		binding.type = bindDesc.Type;
		bindings.push_back(binding);
	}
	return bindings;
}

ComPtr<ID3D12RootSignature> DXCompile::CreateRootSignature(ID3D12Device* device, ID3D12ShaderReflection* vsReflection, ID3D12ShaderReflection* psReflection, std::unordered_map<std::string, uint32_t>& rootParameterMap) {
	assert(device != nullptr);

	rootParameterMap.clear();

	struct ResourceInfo {
		D3D12_SHADER_INPUT_BIND_DESC desc;
		D3D12_SHADER_VISIBILITY visibility;
	};

	std::vector<ResourceInfo> allResources;

	auto CollectResources = [&](ID3D12ShaderReflection* reflection, D3D12_SHADER_VISIBILITY visibility) {
		if (!reflection) return;
		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);
		for (UINT i = 0; i < shaderDesc.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC bindDesc;
			reflection->GetResourceBindingDesc(i, &bindDesc);

			// 重複チェック
			bool found = false;
			for (auto& res : allResources) {
				if (res.desc.BindPoint == bindDesc.BindPoint && res.desc.Type == bindDesc.Type && res.desc.Space == bindDesc.Space) {
					res.visibility = D3D12_SHADER_VISIBILITY_ALL;
					found = true;
					break;
				}
			}
			if (!found) {
				allResources.push_back({ bindDesc, visibility });
			}
		}
	};

	CollectResources(vsReflection, D3D12_SHADER_VISIBILITY_VERTEX);
	CollectResources(psReflection, D3D12_SHADER_VISIBILITY_PIXEL);

	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE>> ranges; // Table用のRange保持用

	Logger::Log("--- Generated Root Signature Layout ---");

	for (size_t i = 0; i < allResources.size(); ++i) {
		const auto& res = allResources[i];
		std::string typeStr;
		char regPrefix = ' ';

		if (res.desc.Type == D3D_SIT_CBUFFER) {
			typeStr = "CBV";
			regPrefix = 'b';
			D3D12_ROOT_PARAMETER param{};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = res.visibility;
			param.Descriptor.ShaderRegister = res.desc.BindPoint;
			param.Descriptor.RegisterSpace = res.desc.Space;
			rootParams.push_back(param);
		} else if (res.desc.Type == D3D_SIT_TEXTURE || res.desc.Type == D3D_SIT_UAV_RWSTRUCTURED) {
			typeStr = (res.desc.Type == D3D_SIT_TEXTURE) ? "SRV (Table)" : "UAV (Table)";
			regPrefix = (res.desc.Type == D3D_SIT_TEXTURE) ? 't' : 'u';
			
			auto range = std::make_unique<D3D12_DESCRIPTOR_RANGE>();
			range->RangeType = (res.desc.Type == D3D_SIT_TEXTURE) ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV : D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			range->NumDescriptors = (res.desc.BindCount == 0) ? 4096 : res.desc.BindCount;
			range->BaseShaderRegister = res.desc.BindPoint;
			range->RegisterSpace = res.desc.Space;
			range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER param{};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.ShaderVisibility = res.visibility;
			param.DescriptorTable.NumDescriptorRanges = 1;
			param.DescriptorTable.pDescriptorRanges = range.get();
			
			ranges.push_back(std::move(range));
			rootParams.push_back(param);
		}

		// インデックスと変数名のマッピングをログ出力
		Logger::Log(std::format("[RootIndex: {}] Name: {}, Type: {}, Register: {}{}, Space: {}",
			i, res.desc.Name, typeStr, regPrefix, res.desc.BindPoint, res.desc.Space));

		rootParameterMap[res.desc.Name] = static_cast<uint32_t>(i);
		rootParameterMap[std::format("{}{}", regPrefix, res.desc.BindPoint)] = static_cast<uint32_t>(i);
	}
	Logger::Log("---------------------------------------");

	// Static Sampler (s0)
	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.pParameters = rootParams.data();
	rootDesc.NumParameters = (UINT)rootParams.size();
	rootDesc.pStaticSamplers = &staticSampler;
	rootDesc.NumStaticSamplers = 1;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) Logger::Log((char*)errorBlob->GetBufferPointer());
		assert(false);
	}

	ComPtr<ID3D12RootSignature> rootSignature;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	return rootSignature;
}
