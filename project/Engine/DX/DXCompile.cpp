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

		if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED) {
			continue;
		}

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

ComPtr<ID3D12RootSignature> DXCompile::CreateRootSignature(
	ID3D12Device* device,
	ID3D12ShaderReflection* vsReflection,
	ID3D12ShaderReflection* psReflection,
	std::unordered_map<std::string, uint32_t>& rootParameterMap,
	D3D12_SHADER_VISIBILITY vsVisibility,
	D3D12_SHADER_VISIBILITY psVisibility,
	const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers) {
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
			allResources.push_back({ bindDesc, visibility });
		}
	};

	CollectResources(vsReflection, vsVisibility);
	CollectResources(psReflection, psVisibility);

	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE>> ranges; // Table用のRange保持用

	Logger::Log("--- Generated Root Signature Layout ---");

	uint32_t currentRootIndex = 0;
	for (const auto& res : allResources) {
		std::string typeStr;
		char regPrefix = ' ';
		bool isAdded = false;

		// リソースの種別判定用
		bool isCBV = false;
		bool isUAV = false;
		D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

		if (res.desc.Type == D3D_SIT_CBUFFER) {
			typeStr = "CBV";
			regPrefix = 'b';
			isCBV = true;
		} else if (res.desc.Type == D3D_SIT_TEXTURE || 
				   res.desc.Type == D3D_SIT_UAV_RWSTRUCTURED || 
				   res.desc.Type == D3D_SIT_UAV_RWTYPED ||
				   res.desc.Type == D3D_SIT_UAV_RWBYTEADDRESS ||
				   res.desc.Type == D3D_SIT_STRUCTURED ||
				   res.desc.Type == D3D_SIT_BYTEADDRESS) {

			isUAV = (res.desc.Type == D3D_SIT_UAV_RWSTRUCTURED || res.desc.Type == D3D_SIT_UAV_RWTYPED || res.desc.Type == D3D_SIT_UAV_RWBYTEADDRESS);
			typeStr = isUAV ? "UAV (Table)" : "SRV (Table)";
			regPrefix = isUAV ? 'u' : 't';
			rangeType = isUAV ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV : D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		} else {
			continue; // 処理対象外
		}

		// 既に同じレジスタ番号のRootParameterが存在するか検索する
		int duplicateIndex = -1;
		for (size_t i = 0; i < rootParams.size(); ++i) {
			const auto& param = rootParams[i];
			if (isCBV && param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
				if (param.Descriptor.ShaderRegister == res.desc.BindPoint &&
					param.Descriptor.RegisterSpace == res.desc.Space) {
					duplicateIndex = static_cast<int>(i);
					break;
				}
			} else if (!isCBV && param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
				const auto* range = param.DescriptorTable.pDescriptorRanges;
				if (range->RangeType == rangeType &&
					range->BaseShaderRegister == res.desc.BindPoint &&
					range->RegisterSpace == res.desc.Space) {
					duplicateIndex = static_cast<int>(i);
					break;
				}
			}
		}

		// 重複が見つかった場合は新規追加せず、VisibilityをALLに拡張する
		if (duplicateIndex != -1) {
			rootParams[duplicateIndex].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			
			Logger::Log(std::format("[RootIndex: {}] Merged (Visibility=ALL) Name: {}, Type: {}, Register: {}{}, Space: {}",
				duplicateIndex, res.desc.Name, typeStr, regPrefix, res.desc.BindPoint, res.desc.Space));

			// PSとVSで変数名が異なるケースも考慮してマッピングを追加
			rootParameterMap[res.desc.Name] = duplicateIndex;
			rootParameterMap[std::format("{}{}", regPrefix, res.desc.BindPoint)] = duplicateIndex;
			continue; // これ以上の処理（新規追加）はスキップ
		}

		// 見つからなかった場合は新規パラメータとして登録
		if (isCBV) {
			D3D12_ROOT_PARAMETER param{};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = res.visibility;
			param.Descriptor.ShaderRegister = res.desc.BindPoint;
			param.Descriptor.RegisterSpace = res.desc.Space;
			rootParams.push_back(param);
			isAdded = true;
		} else {
			auto range = std::make_unique<D3D12_DESCRIPTOR_RANGE>();
			range->RangeType = rangeType;
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
			isAdded = true;
		}

		if (isAdded) {
			// インデックスと変数名のマッピングをログ出力
			Logger::Log(std::format("[RootIndex: {}] Name: {}, Type: {}, Register: {}{}, Space: {}",
				currentRootIndex, res.desc.Name, typeStr, regPrefix, res.desc.BindPoint, res.desc.Space));

			rootParameterMap[res.desc.Name] = currentRootIndex;
			rootParameterMap[std::format("{}{}", regPrefix, res.desc.BindPoint)] = currentRootIndex;
			currentRootIndex++;
		}
	}
	Logger::Log("---------------------------------------");


	const D3D12_STATIC_SAMPLER_DESC* pSamplers = nullptr;
	UINT numSamplers = 0;
	D3D12_STATIC_SAMPLER_DESC defaultSampler{};
	if (staticSamplers.empty()) {
		defaultSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		defaultSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		defaultSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		defaultSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		defaultSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		defaultSampler.MaxLOD = D3D12_FLOAT32_MAX;
		defaultSampler.ShaderRegister = 0;
		// PSがある場合はPS限定、それ以外（CS等）はALL
		defaultSampler.ShaderVisibility = (psReflection != nullptr && psVisibility == D3D12_SHADER_VISIBILITY_PIXEL) ? D3D12_SHADER_VISIBILITY_PIXEL : D3D12_SHADER_VISIBILITY_ALL;

		pSamplers = &defaultSampler;
		numSamplers = 1;
	} else {
		pSamplers = staticSamplers.data();
		numSamplers = (UINT)staticSamplers.size();
	}
	
	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.pParameters = rootParams.data();
	rootDesc.NumParameters = (UINT)rootParams.size();
	rootDesc.pStaticSamplers = pSamplers;
	rootDesc.NumStaticSamplers = numSamplers;
	
	// CSの場合は InputAssembler 許可フラグを立てない
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	if (vsVisibility == D3D12_SHADER_VISIBILITY_ALL) {
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	}

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

ComPtr<ID3D12RootSignature> DXCompile::CreateRootSignature(ID3D12Device* device, ID3D12ShaderReflection* csReflection, std::unordered_map<std::string, uint32_t>& rootParameterMap, const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers) {
	// CSの場合は visibility を ALL にして共通関数を呼ぶ
	return CreateRootSignature(device, csReflection, nullptr, rootParameterMap, D3D12_SHADER_VISIBILITY_ALL, D3D12_SHADER_VISIBILITY_ALL, staticSamplers);
}
