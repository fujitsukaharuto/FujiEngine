#include "Engine/DXC/Device/GraphicsDevice.h"
#include <cassert>
#include "Engine/Logger/Logger.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")



using namespace DXC;
using namespace Logger;



void GraphicsDevice::Initialize() {
#ifdef _DEBUG
	debugController_ = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		debugController_->EnableDebugLayer();
		debugController_->SetEnableGPUBasedValidation(true);
	}
#endif // _DEBUG

	//DXGIファクトリーの作成
	dxgiFactory_ = nullptr;
	//HRESULTはWindows系のエラーコードで、関数が成功したかどうかをマクロで判断できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	//初期化の根本的な部分でエラーが出た場合は間違えているか、どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	useAdapter_ = nullptr;
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			Logger::Log((std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter_ = nullptr;//ソフトウェアタブの場合見つからなかったことに
	}
	assert(useAdapter_ != nullptr);

	device_ = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr)) {
			Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device_ != nullptr);
	Logger::Log("Complete create D3D12Device!!\n");

	// 非対応環境でも起動は続けたいので、失敗しても nullptr のままにする
	if (FAILED(device_.As(&device5_))) {
		device5_ = nullptr;
		Logger::Log("ID3D12Device5 is not available. Raytracing will be disabled.\n");
	}

	QueryFeatureSupport();

#ifdef _DEBUGMODE
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);

		infoQueue->Release();
	}
#endif // _DEBUG
}

void GraphicsDevice::QueryFeatureSupport() {
	// RayQuery は 1.1 から。1.0 は DispatchRays + SBT だけなので非対応と同じ扱い
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
		raytracingTier_ = options5.RaytracingTier;
	}

	// 値は 10 = 1.0 / 11 = 1.1 …と10倍。switchで列挙すると新しいTierを誤表示するので数値から作る
	const uint32_t tierValue = static_cast<uint32_t>(raytracingTier_);
	const std::string tierString = (raytracingTier_ == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		? std::string("NotSupported")
		: std::format("{}.{}", tierValue / 10, tierValue % 10);
	Logger::Log(std::format("RaytracingTier : {}\n", tierString));

	// 「アプリが知っている最大値」を渡す仕様で、知らない値だと E_INVALIDARG になるので高い順に試す
	constexpr D3D_SHADER_MODEL kCandidates[] = {
		D3D_SHADER_MODEL_6_6, D3D_SHADER_MODEL_6_5, D3D_SHADER_MODEL_6_0
	};
	for (D3D_SHADER_MODEL candidate : kCandidates) {
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{ candidate };
		if (SUCCEEDED(device_->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))) {
			highestShaderModel_ = shaderModel.HighestShaderModel;
			break;
		}
	}
	Logger::Log(std::format("HighestShaderModel : {}.{}\n",
		(static_cast<uint32_t>(highestShaderModel_) >> 4) & 0xf,
		static_cast<uint32_t>(highestShaderModel_) & 0xf));

	Logger::Log(std::format("RayQuery (inline raytracing) : {}\n", IsRayQuerySupported() ? "Supported" : "NOT supported"));
}

void GraphicsDevice::Finalize() {
	device5_.Reset();
	device_.Reset();
	useAdapter_.Reset();
	dxgiFactory_.Reset();
#ifdef _DEBUGMODE
	debugController_.Reset();
#endif
}
