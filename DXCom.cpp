#include "DXCom.h"
#include "Fuji.h"
#include "ImGuiManager.h"
#include "Input.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <fstream>
#include <sstream>
#include <random>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

DXCom* DXCom::GetInstance()
{
	static DXCom instance;
	return &instance;
}

void DXCom::InitDX(MyWin* myWin)
{
	assert(myWin);

	myWin_ = myWin;


	CreateDXGI();
	InitializeCommand();
	CreateSwapChain();
	CreateRenderTargets();
	CreateDepthBuffer();
	CreateFence();


	SettingDxcUtil();
	SettingIncludeHandle();
	SettingRootSignature();
	SettingGraphicPipeline();
	SettingVertex();
	SettingSpriteVertex();
	SettingResource();
	SettingSpriteResource();
	//SettingTexture();
	//SettingImgui();
}

void DXCom::CreateDXGI()
{

#ifdef _DEBUG
	debugController_ = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_))))
	{
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
		IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
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
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr))
		{
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device_ != nullptr);
	Log("Complete creat D3D12Device!!\n");

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
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

void DXCom::InitializeCommand()
{

	commandQueue_ = nullptr;
	HRESULT hr = device_->CreateCommandQueue(&commandQueueDesc_,
		IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));

	commandAllocator_ = nullptr;
	hr = device_->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

	commandList_ = nullptr;
	hr = device_->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));

}

void DXCom::CreateSwapChain()
{
	swapChain_ = nullptr;
	swapChainDesc_.Width = MyWin::kWindowWidth;
	swapChainDesc_.Height = MyWin::kWindowHeight;
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc_.SampleDesc.Count = 1;
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc_.BufferCount = 2;
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(), myWin_->GetHwnd(), &swapChainDesc_,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));


}

void DXCom::CreateRenderTargets()
{
	rtvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);

	HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));


	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	rtvHandles_[0] = rtvStartHandle;
	device_->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc_, rtvHandles_[0]);
	rtvHandles_[1].ptr = rtvHandles_[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device_->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc_, rtvHandles_[1]);

	rtvHandles_[2].ptr = rtvHandles_[1].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	offscreenrtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	offscreenrtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	clearColorValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	clearColorValue.Color[0] = 0.1f;
	clearColorValue.Color[1] = 0.25f;
	clearColorValue.Color[2] = 0.5f;
	clearColorValue.Color[3] = 1.0f;

	offscreenrt_ = CreateOffscreenTextureResource(
		device_.Get(), MyWin::kWindowWidth, MyWin::kWindowHeight, clearColorValue);

	device_->CreateRenderTargetView(offscreenrt_.Get(), &offscreenrtvDesc_, rtvHandles_[2]);

}

void DXCom::CreateDepthBuffer()
{

	depthStencilResource_ = CreateDepthStencilTextureResource(
		device_.Get(), MyWin::kWindowWidth, MyWin::kWindowHeight);

	dsvDescriptorHeap_ = CreateDescriptorHeap(
		device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	dsvDesc_.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc_.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc_, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());


}

void DXCom::CreateFence()
{
	fence_ = nullptr;
	fenceValue_ = 0;
	HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

}


void DXCom::SettingDxcUtil()
{
	dxcUtils_ = nullptr;
	dxcCompiler_ = nullptr;
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));
}

void DXCom::SettingIncludeHandle()
{
	includeHandler_ = nullptr;
	HRESULT hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

void DXCom::SettingRootSignature()
{
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);


	signatureBlob_ = nullptr;
	errorBlob_ = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		assert(false);
	}


	rootSignature_ = nullptr;
	hr = device_->CreateRootSignature(0, signatureBlob_->GetBufferPointer(),
		signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	if (isGrayscale_)
	{
		D3D12_ROOT_SIGNATURE_DESC rootSignatureGrayDesc;
		rootSignatureGrayDesc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		CD3DX12_DESCRIPTOR_RANGE rangeGray[1] = {};
		rangeGray[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		D3D12_ROOT_PARAMETER rootParametersGray[1] = {};
		rootParametersGray[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParametersGray[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParametersGray[0].DescriptorTable.pDescriptorRanges = rangeGray;
		rootParametersGray[0].DescriptorTable.NumDescriptorRanges = _countof(rangeGray);

		D3D12_STATIC_SAMPLER_DESC samplersGray[1] = {};
		samplersGray[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplersGray[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersGray[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersGray[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersGray[0].MipLODBias = 0;
		samplersGray[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplersGray[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplersGray[0].ShaderRegister = 0;
		samplersGray[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootSignatureGrayDesc.pParameters = rootParametersGray;
		rootSignatureGrayDesc.NumParameters = _countof(rootParametersGray);
		rootSignatureGrayDesc.pStaticSamplers = samplersGray;
		rootSignatureGrayDesc.NumStaticSamplers = _countof(samplersGray);

		signatureGrayBlob_ = nullptr;
		errorGrayBlob_ = nullptr;
		hr = D3D12SerializeRootSignature(&rootSignatureGrayDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signatureGrayBlob_, &errorGrayBlob_);

		if (FAILED(hr))
		{
			Log(reinterpret_cast<char*>(errorGrayBlob_->GetBufferPointer()));
			assert(false);
		}

		hr = device_->CreateRootSignature(0, signatureGrayBlob_->GetBufferPointer(),
			signatureGrayBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignatureGrayscale_));

		assert(SUCCEEDED(hr));
	}

	if (isMetaBall_)
	{
		D3D12_ROOT_SIGNATURE_DESC rootSignatureMetaDesc;
		rootSignatureMetaDesc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		CD3DX12_DESCRIPTOR_RANGE rangeMeta[1] = {};
		rangeMeta[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		D3D12_ROOT_PARAMETER rootParametersMeta[2] = {};
		rootParametersMeta[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParametersMeta[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParametersMeta[0].DescriptorTable.pDescriptorRanges = rangeMeta;
		rootParametersMeta[0].DescriptorTable.NumDescriptorRanges = _countof(rangeMeta);

		rootParametersMeta[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParametersMeta[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParametersMeta[1].Descriptor.ShaderRegister = 0;

		D3D12_STATIC_SAMPLER_DESC samplersMeta[1] = {};
		samplersMeta[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplersMeta[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersMeta[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersMeta[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersMeta[0].MipLODBias = 0;
		samplersMeta[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplersMeta[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplersMeta[0].ShaderRegister = 0;
		samplersMeta[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootSignatureMetaDesc.pParameters = rootParametersMeta;
		rootSignatureMetaDesc.NumParameters = _countof(rootParametersMeta);
		rootSignatureMetaDesc.pStaticSamplers = samplersMeta;
		rootSignatureMetaDesc.NumStaticSamplers = _countof(samplersMeta);

		signatureMetaBlob_ = nullptr;
		errorMetaBlob_ = nullptr;
		hr = D3D12SerializeRootSignature(&rootSignatureMetaDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signatureMetaBlob_, &errorMetaBlob_);

		if (FAILED(hr))
		{
			Log(reinterpret_cast<char*>(errorMetaBlob_->GetBufferPointer()));
			assert(false);
		}

		hr = device_->CreateRootSignature(0, signatureMetaBlob_->GetBufferPointer(),
			signatureMetaBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignatureMetaBall_));

		assert(SUCCEEDED(hr));
	}

	if (isGaussian_)
	{
		D3D12_ROOT_SIGNATURE_DESC rootSignatureGaussDesc;
		rootSignatureGaussDesc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		CD3DX12_DESCRIPTOR_RANGE rangeGauss[1] = {};
		rangeGauss[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		D3D12_ROOT_PARAMETER rootParametersGauss[1] = {};
		rootParametersGauss[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParametersGauss[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParametersGauss[0].DescriptorTable.pDescriptorRanges = rangeGauss;
		rootParametersGauss[0].DescriptorTable.NumDescriptorRanges = _countof(rangeGauss);

		D3D12_STATIC_SAMPLER_DESC samplersGauss[1] = {};
		samplersGauss[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplersGauss[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersGauss[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersGauss[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersGauss[0].MipLODBias = 0;
		samplersGauss[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplersGauss[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplersGauss[0].ShaderRegister = 0;
		samplersGauss[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootSignatureGaussDesc.pParameters = rootParametersGauss;
		rootSignatureGaussDesc.NumParameters = _countof(rootParametersGauss);
		rootSignatureGaussDesc.pStaticSamplers = samplersGauss;
		rootSignatureGaussDesc.NumStaticSamplers = _countof(samplersGauss);

		signatureGaussBlob_ = nullptr;
		errorGaussBlob_ = nullptr;
		hr = D3D12SerializeRootSignature(&rootSignatureGaussDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signatureGaussBlob_, &errorGaussBlob_);

		if (FAILED(hr))
		{
			Log(reinterpret_cast<char*>(errorGaussBlob_->GetBufferPointer()));
			assert(false);
		}

		hr = device_->CreateRootSignature(0, signatureGaussBlob_->GetBufferPointer(),
			signatureGaussBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignatureGauss_));

		assert(SUCCEEDED(hr));
	}


	if (isNonePost_)
	{
		D3D12_ROOT_SIGNATURE_DESC rootSignatureNoneDesc;
		rootSignatureNoneDesc.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		CD3DX12_DESCRIPTOR_RANGE rangeNone[1] = {};
		rangeNone[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		D3D12_ROOT_PARAMETER rootParametersNone[1] = {};
		rootParametersNone[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParametersNone[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParametersNone[0].DescriptorTable.pDescriptorRanges = rangeNone;
		rootParametersNone[0].DescriptorTable.NumDescriptorRanges = _countof(rangeNone);

		D3D12_STATIC_SAMPLER_DESC samplersNone[1] = {};
		samplersNone[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplersNone[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersNone[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersNone[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplersNone[0].MipLODBias = 0;
		samplersNone[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplersNone[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplersNone[0].ShaderRegister = 0;
		samplersNone[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		rootSignatureNoneDesc.pParameters = rootParametersNone;
		rootSignatureNoneDesc.NumParameters = _countof(rootParametersNone);
		rootSignatureNoneDesc.pStaticSamplers = samplersNone;
		rootSignatureNoneDesc.NumStaticSamplers = _countof(samplersNone);

		signatureNoneBlob_ = nullptr;
		errorNoneBlob_ = nullptr;
		hr = D3D12SerializeRootSignature(&rootSignatureNoneDesc,
			D3D_ROOT_SIGNATURE_VERSION_1, &signatureNoneBlob_, &errorNoneBlob_);

		if (FAILED(hr))
		{
			Log(reinterpret_cast<char*>(errorNoneBlob_->GetBufferPointer()));
			assert(false);
		}

		hr = device_->CreateRootSignature(0, signatureNoneBlob_->GetBufferPointer(),
			signatureNoneBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignatureNone_));

		assert(SUCCEEDED(hr));
	}

}

void DXCom::SettingGraphicPipeline()
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	/*D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC rtb{};
	rtb.BlendEnable = true;
	rtb.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtb.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtb.BlendOp = D3D12_BLEND_OP_ADD;
	rtb.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtb.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtb.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtb.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	blendDesc.RenderTarget[0] = rtb;*/

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;


	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	vertexShaderBlob_ = CompileShader(L"Object3d.VS.hlsl",
		L"vs_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
	assert(vertexShaderBlob_ != nullptr);

	pixelShaderBlob_ = CompileShader(L"Object3d.PS.hlsl",
		L"ps_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
	assert(pixelShaderBlob_ != nullptr);


	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	graphicPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(),
	vertexShaderBlob_->GetBufferSize() };
	graphicPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(),
	pixelShaderBlob_->GetBufferSize() };

	graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	graphicPipelineStateDesc.BlendState = blendDesc;
	graphicPipelineStateDesc.RasterizerState = rasterizerDesc;

	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	graphicPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	graphicsPipelineState_ = nullptr;
	HRESULT hr = device_->CreateGraphicsPipelineState(&graphicPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState_));
	assert(SUCCEEDED(hr));


	if (isGrayscale_)
	{
		D3D12_INPUT_ELEMENT_DESC inputLayoutGray[2] = {};
		inputLayoutGray[0].SemanticName = "POSITION";
		inputLayoutGray[0].SemanticIndex = 0;
		inputLayoutGray[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputLayoutGray[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputLayoutGray[1].SemanticName = "TEXCOORD";
		inputLayoutGray[1].SemanticIndex = 0;
		inputLayoutGray[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputLayoutGray[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_LAYOUT_DESC inputLayoutGrayDesc{};
		inputLayoutGrayDesc.pInputElementDescs = inputLayoutGray;
		inputLayoutGrayDesc.NumElements = _countof(inputLayoutGray);

		vertexShaderGrayBlob_ = CompileShader(L"SimpleGrayscale.VS.hlsl",
			L"vs_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(vertexShaderGrayBlob_ != nullptr);

		pixelShaderGrayBlob_ = CompileShader(L"SimpleGrayscale.PS.hlsl",
			L"ps_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(pixelShaderGrayBlob_ != nullptr);


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoGrayDesc = {};
		psoGrayDesc.InputLayout = inputLayoutGrayDesc;
		psoGrayDesc.pRootSignature = rootSignatureGrayscale_.Get();
		psoGrayDesc.VS = { vertexShaderGrayBlob_->GetBufferPointer(),
		vertexShaderGrayBlob_->GetBufferSize() };
		psoGrayDesc.PS = { pixelShaderGrayBlob_->GetBufferPointer(),
		pixelShaderGrayBlob_->GetBufferSize() };
		psoGrayDesc.DepthStencilState.DepthEnable = FALSE;
		psoGrayDesc.DepthStencilState.StencilEnable = FALSE;
		psoGrayDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoGrayDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoGrayDesc.SampleMask = UINT_MAX;
		psoGrayDesc.PrimitiveTopologyType =
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoGrayDesc.NumRenderTargets = 1;
		psoGrayDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoGrayDesc.SampleDesc.Count = 1;

		graphicsPipelineStateGary_ = nullptr;
		hr = device_->CreateGraphicsPipelineState(&psoGrayDesc,
			IID_PPV_ARGS(&graphicsPipelineStateGary_));
		assert(SUCCEEDED(hr));
	}


	if (isMetaBall_)
	{
		D3D12_INPUT_ELEMENT_DESC inputLayoutMeta[2] = {};
		inputLayoutMeta[0].SemanticName = "POSITION";
		inputLayoutMeta[0].SemanticIndex = 0;
		inputLayoutMeta[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputLayoutMeta[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputLayoutMeta[1].SemanticName = "TEXCOORD";
		inputLayoutMeta[1].SemanticIndex = 0;
		inputLayoutMeta[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputLayoutMeta[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_LAYOUT_DESC inputLayoutMetaDesc{};
		inputLayoutMetaDesc.pInputElementDescs = inputLayoutMeta;
		inputLayoutMetaDesc.NumElements = _countof(inputLayoutMeta);

		vertexShaderMetaBlob_ = CompileShader(L"MetaBall.VS.hlsl",
			L"vs_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(vertexShaderMetaBlob_ != nullptr);

		pixelShaderMetaBlob_ = CompileShader(L"MetaBall.PS.hlsl",
			L"ps_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(pixelShaderMetaBlob_ != nullptr);


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoMetaDesc = {};
		psoMetaDesc.InputLayout = inputLayoutMetaDesc;
		psoMetaDesc.pRootSignature = rootSignatureMetaBall_.Get();
		psoMetaDesc.VS = { vertexShaderMetaBlob_->GetBufferPointer(),
		vertexShaderMetaBlob_->GetBufferSize() };
		psoMetaDesc.PS = { pixelShaderMetaBlob_->GetBufferPointer(),
		pixelShaderMetaBlob_->GetBufferSize() };
		psoMetaDesc.DepthStencilState.DepthEnable = FALSE;
		psoMetaDesc.DepthStencilState.StencilEnable = FALSE;
		psoMetaDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoMetaDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoMetaDesc.SampleMask = UINT_MAX;
		psoMetaDesc.PrimitiveTopologyType =
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoMetaDesc.NumRenderTargets = 1;
		psoMetaDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoMetaDesc.SampleDesc.Count = 1;

		graphicsPipelineStateMeta_ = nullptr;
		hr = device_->CreateGraphicsPipelineState(&psoMetaDesc,
			IID_PPV_ARGS(&graphicsPipelineStateMeta_));
		assert(SUCCEEDED(hr));
	}


	if (isGaussian_)
	{
		D3D12_INPUT_ELEMENT_DESC inputLayoutGauss[2] = {};
		inputLayoutGauss[0].SemanticName = "POSITION";
		inputLayoutGauss[0].SemanticIndex = 0;
		inputLayoutGauss[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputLayoutGauss[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputLayoutGauss[1].SemanticName = "TEXCOORD";
		inputLayoutGauss[1].SemanticIndex = 0;
		inputLayoutGauss[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputLayoutGauss[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_LAYOUT_DESC inputLayoutGaussDesc{};
		inputLayoutGaussDesc.pInputElementDescs = inputLayoutGauss;
		inputLayoutGaussDesc.NumElements = _countof(inputLayoutGauss);

		vertexShaderGaussBlob_ = CompileShader(L"GaussianBlur.VS.hlsl",
			L"vs_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(vertexShaderGaussBlob_ != nullptr);

		pixelShaderGaussBlob_ = CompileShader(L"GaussianBlur.PS.hlsl",
			L"ps_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(pixelShaderGaussBlob_ != nullptr);


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoGaussDesc = {};
		psoGaussDesc.InputLayout = inputLayoutGaussDesc;
		psoGaussDesc.pRootSignature = rootSignatureGauss_.Get();
		psoGaussDesc.VS = { vertexShaderGaussBlob_->GetBufferPointer(),
		vertexShaderGaussBlob_->GetBufferSize() };
		psoGaussDesc.PS = { pixelShaderGaussBlob_->GetBufferPointer(),
		pixelShaderGaussBlob_->GetBufferSize() };
		psoGaussDesc.DepthStencilState.DepthEnable = FALSE;
		psoGaussDesc.DepthStencilState.StencilEnable = FALSE;
		psoGaussDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoGaussDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoGaussDesc.SampleMask = UINT_MAX;
		psoGaussDesc.PrimitiveTopologyType =
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoGaussDesc.NumRenderTargets = 1;
		psoGaussDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoGaussDesc.SampleDesc.Count = 1;

		graphicsPipelineStateGauss_ = nullptr;
		hr = device_->CreateGraphicsPipelineState(&psoGaussDesc,
			IID_PPV_ARGS(&graphicsPipelineStateGauss_));
		assert(SUCCEEDED(hr));
	}


	if (isNonePost_)
	{
		D3D12_INPUT_ELEMENT_DESC inputLayoutNone[2] = {};
		inputLayoutNone[0].SemanticName = "POSITION";
		inputLayoutNone[0].SemanticIndex = 0;
		inputLayoutNone[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputLayoutNone[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputLayoutNone[1].SemanticName = "TEXCOORD";
		inputLayoutNone[1].SemanticIndex = 0;
		inputLayoutNone[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputLayoutNone[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_LAYOUT_DESC inputLayoutNoneDesc{};
		inputLayoutNoneDesc.pInputElementDescs = inputLayoutNone;
		inputLayoutNoneDesc.NumElements = _countof(inputLayoutNone);

		vertexShaderNoneBlob_ = CompileShader(L"NonePost.VS.hlsl",
			L"vs_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(vertexShaderNoneBlob_ != nullptr);

		pixelShaderNoneBlob_ = CompileShader(L"NonePost.PS.hlsl",
			L"ps_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
		assert(pixelShaderNoneBlob_ != nullptr);


		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoNoneDesc = {};
		psoNoneDesc.InputLayout = inputLayoutNoneDesc;
		psoNoneDesc.pRootSignature = rootSignatureNone_.Get();
		psoNoneDesc.VS = { vertexShaderNoneBlob_->GetBufferPointer(),
		vertexShaderNoneBlob_->GetBufferSize() };
		psoNoneDesc.PS = { pixelShaderNoneBlob_->GetBufferPointer(),
		pixelShaderNoneBlob_->GetBufferSize() };
		psoNoneDesc.DepthStencilState.DepthEnable = FALSE;
		psoNoneDesc.DepthStencilState.StencilEnable = FALSE;
		psoNoneDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoNoneDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoNoneDesc.SampleMask = UINT_MAX;
		psoNoneDesc.PrimitiveTopologyType =
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoNoneDesc.NumRenderTargets = 1;
		psoNoneDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoNoneDesc.SampleDesc.Count = 1;

		graphicsPipelineStateNone_ = nullptr;
		hr = device_->CreateGraphicsPipelineState(&psoNoneDesc,
			IID_PPV_ARGS(&graphicsPipelineStateNone_));
		assert(SUCCEEDED(hr));
	}


	vertexGrayResource_ = CreateBufferResource(device_.Get(), sizeof(GrayscaleVertex) * 6);

	vertexGrayBufferView_.BufferLocation = vertexGrayResource_->GetGPUVirtualAddress();
	vertexGrayBufferView_.SizeInBytes = sizeof(GrayscaleVertex) * 6;
	vertexGrayBufferView_.StrideInBytes = sizeof(GrayscaleVertex);

	grayVertexDate_ = nullptr;
	vertexGrayResource_->Map(0, nullptr, reinterpret_cast<void**>(&grayVertexDate_));

	grayVertexDate_[0] = { Vector4(-1.0f,-1.0f,0.0f,1.0f),Vector2(0.0f,1.0f) };
	grayVertexDate_[1] = { Vector4(-1.0f,1.0f,0.0f,1.0f),Vector2(0.0f,0.0f) };
	grayVertexDate_[2] = { Vector4(1.0f,1.0f,0.0f,1.0f),Vector2(1.0f,0.0f) };
	grayVertexDate_[3] = { Vector4(1.0f,-1.0f,0.0f,1.0f),Vector2(1.0f,1.0f) };


	indexGrayResourece_ = CreateBufferResource(device_.Get(), sizeof(uint32_t) * 6);

	indexGrayBufferView_.BufferLocation = indexGrayResourece_->GetGPUVirtualAddress();
	indexGrayBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexGrayBufferView_.Format = DXGI_FORMAT_R32_UINT;

	indexGrayResourece_->Map(0, nullptr, reinterpret_cast<void**>(&indexGrayData_));

	indexGrayData_[0] = 0;
	indexGrayData_[1] = 1;
	indexGrayData_[2] = 2;
	indexGrayData_[3] = 0;
	indexGrayData_[4] = 2;
	indexGrayData_[5] = 3;

	isGrayscale_ = false;
	isNonePost_ = true;
	isMetaBall_ = false;
	isGaussian_ = false;

}


//---------------↓↓↓↓↓↓↓↓改善しなきゃ↓↓↓↓↓↓↓↓--------------------------------------


void DXCom::SettingVertex()
{
	vertexResource_ = CreateBufferResource(device_.Get(), sizeof(VertexData) * 2000);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 2000;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	vertexDate_ = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDate_));

	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;
	const float kLonEvery = (pi * 2.0f) / static_cast<float>(kSubdivision);
	const float kLatEvery = (pi) / static_cast<float>(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	{
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
		{
			float lon = lonIndex * kLonEvery;
			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;

			float u = float(lonIndex) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);

			vertexDate_[startIndex].position.X = cosf(lat) * cosf(lon);
			vertexDate_[startIndex].position.Y = sinf(lat);
			vertexDate_[startIndex].position.Z = cosf(lat) * sinf(lon);
			vertexDate_[startIndex].position.W = 1.0f;
			vertexDate_[startIndex].texcoord = { u,v };

			vertexDate_[startIndex].normal.x = vertexDate_[startIndex].position.X;
			vertexDate_[startIndex].normal.y = vertexDate_[startIndex].position.Y;
			vertexDate_[startIndex].normal.z = vertexDate_[startIndex].position.Z;


			vertexDate_[startIndex + 1].position.X = cosf(lat + kLatEvery) * cosf(lon);
			vertexDate_[startIndex + 1].position.Y = sinf(lat + kLatEvery);
			vertexDate_[startIndex + 1].position.Z = cosf(lat + kLatEvery) * sinf(lon);
			vertexDate_[startIndex + 1].position.W = 1.0f;
			vertexDate_[startIndex + 1].texcoord = { u,v - (float(1.0f) / float(kSubdivision)) };

			vertexDate_[startIndex + 1].normal.x = vertexDate_[startIndex + 1].position.X;
			vertexDate_[startIndex + 1].normal.y = vertexDate_[startIndex + 1].position.Y;
			vertexDate_[startIndex + 1].normal.z = vertexDate_[startIndex + 1].position.Z;


			vertexDate_[startIndex + 2].position.X = cosf(lat) * cosf(lon + kLonEvery);
			vertexDate_[startIndex + 2].position.Y = sinf(lat);
			vertexDate_[startIndex + 2].position.Z = cosf(lat) * sinf(lon + kLonEvery);
			vertexDate_[startIndex + 2].position.W = 1.0f;
			vertexDate_[startIndex + 2].texcoord = { u + (float(1.0f) / float(kSubdivision)),v };

			vertexDate_[startIndex + 2].normal.x = vertexDate_[startIndex + 2].position.X;
			vertexDate_[startIndex + 2].normal.y = vertexDate_[startIndex + 2].position.Y;
			vertexDate_[startIndex + 2].normal.z = vertexDate_[startIndex + 2].position.Z;


			vertexDate_[startIndex + 3].position.X = cosf(lat + kLatEvery) * cosf(lon);
			vertexDate_[startIndex + 3].position.Y = sinf(lat + kLatEvery);
			vertexDate_[startIndex + 3].position.Z = cosf(lat + kLatEvery) * sinf(lon);
			vertexDate_[startIndex + 3].position.W = 1.0f;
			vertexDate_[startIndex + 3].texcoord = { u,v - (float(1.0f) / float(kSubdivision)) };

			vertexDate_[startIndex + 3].normal.x = vertexDate_[startIndex + 3].position.X;
			vertexDate_[startIndex + 3].normal.y = vertexDate_[startIndex + 3].position.Y;
			vertexDate_[startIndex + 3].normal.z = vertexDate_[startIndex + 3].position.Z;


			vertexDate_[startIndex + 4].position.X = cosf(lat + kLatEvery) * cosf(lon + kLonEvery);
			vertexDate_[startIndex + 4].position.Y = sinf(lat + kLatEvery);
			vertexDate_[startIndex + 4].position.Z = cosf(lat + kLatEvery) * sinf(lon + kLonEvery);
			vertexDate_[startIndex + 4].position.W = 1.0f;
			vertexDate_[startIndex + 4].texcoord = { u + (float(1.0f) / float(kSubdivision)),v - (float(1.0f) / float(kSubdivision)) };

			vertexDate_[startIndex + 4].normal.x = vertexDate_[startIndex + 4].position.X;
			vertexDate_[startIndex + 4].normal.y = vertexDate_[startIndex + 4].position.Y;
			vertexDate_[startIndex + 4].normal.z = vertexDate_[startIndex + 4].position.Z;


			vertexDate_[startIndex + 5].position.X = cosf(lat) * cosf(lon + kLonEvery);
			vertexDate_[startIndex + 5].position.Y = sinf(lat);
			vertexDate_[startIndex + 5].position.Z = cosf(lat) * sinf(lon + kLonEvery);
			vertexDate_[startIndex + 5].position.W = 1.0f;
			vertexDate_[startIndex + 5].texcoord = { u + (float(1.0f) / float(kSubdivision)),v };

			vertexDate_[startIndex + 5].normal.x = vertexDate_[startIndex + 5].position.X;
			vertexDate_[startIndex + 5].normal.y = vertexDate_[startIndex + 5].position.Y;
			vertexDate_[startIndex + 5].normal.z = vertexDate_[startIndex + 5].position.Z;

		}
	}

	/*vertexResource2_ = CreateBufferResource(device_.Get(), sizeof(VertexData) * 3);

	vertexBufferView2_.BufferLocation = vertexResource2_->GetGPUVirtualAddress();
	vertexBufferView2_.SizeInBytes = sizeof(VertexData) * 3;
	vertexBufferView2_.StrideInBytes = sizeof(VertexData);

	vertexDate2_ = nullptr;
	vertexResource2_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDate2_));

	vertexDate2_[0] = { { -0.5f,-0.5f,0.5f,1.0f }, { 0.0f,1.0f }, { 0.0f,0.0f,-1.0f } };
	vertexDate2_[1] = { { 0.0f,0.0f,0.0f,1.0f }, { 0.5f,0.0f }, { 0.0f,0.0f,-1.0f } };
	vertexDate2_[2] = { { 0.5f,-0.5f,-0.5f,1.0f }, { 1.0f,1.0f }, { 0.0f,0.0f,-1.0f } };*/

	/*std::random_device seed;
	std::mt19937 random(seed());
	std::uniform_real_distribution<> number(-3.5, 3.5);
	std::uniform_real_distribution<> velXZNumber(-0.05f, 0.05f);
	std::uniform_real_distribution<> velYNumber(0.01f, 0.06f);
	std::uniform_real_distribution<> rotNumber(-0.1f, 0.1f);

	for (int i = 0; i < particleIndex; i++)
	{
		vertexParticleResource_[i] = CreateBufferResource(device_, sizeof(VertexData) * 3);

		vertexParticleBufferView_[i].BufferLocation = vertexParticleResource_[i]->GetGPUVirtualAddress();
		vertexParticleBufferView_[i].SizeInBytes = sizeof(VertexData) * 3;
		vertexParticleBufferView_[i].StrideInBytes = sizeof(VertexData);

		vertexParticleDate_[i] = nullptr;
		vertexParticleResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&vertexParticleDate_[i]));

		vertexParticleDate_[i][0] = { { -0.3f,-0.3f,0.0f,1.0f }, { 0.0f,1.0f }, { 0.0f,0.0f,-1.0f } };
		vertexParticleDate_[i][1] = { { 0.0f,0.3f,0.0f,1.0f }, { 0.5f,0.0f }, { 0.0f,0.0f,-1.0f } };
		vertexParticleDate_[i][2] = { { 0.3f,-0.3f,0.0f,1.0f }, { 1.0f,1.0f }, { 0.0f,0.0f,-1.0f } };


		wvpParticleResource_[i] = CreateBufferResource(device_, sizeof(TransformationMatrix));
		wvpParticleDate_[i] = nullptr;
		wvpParticleResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpParticleDate_[i]));
		wvpParticleDate_[i]->WVP = MakeIdentity4x4();
		wvpParticleDate_[i]->World = MakeIdentity4x4();

		materialParticleResource_[i] = CreateBufferResource(device_, sizeof(Material));
		materialParticleDate_[i] = nullptr;
		materialParticleResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&materialParticleDate_[i]));
		materialParticleDate_[i]->color = particleColor;
		materialParticleDate_[i]->enableLighting = false;
		materialParticleDate_[i]->uvTransform = MakeIdentity4x4();

		float randomx = float(number(random));
		float randomz = float(number(random));

		transformParticle[i] = { {0.5f,0.5f,0.5f},{0.0f,0.0f,0.0f},{randomx,-2.5f,randomz} };

		float randomvelx = float(velXZNumber(random));
		float randomvely = float(velYNumber(random));
		float randomvelz = float(velXZNumber(random));

		float randomRotx = float(rotNumber(random));
		float randomRoty = float(rotNumber(random));
		float randomRotz = float(rotNumber(random));

		particleVel[i] = { randomvelx,randomvely,randomvelz };
		particleRot[i] = { randomRotx,randomRoty,randomRotz };
	}*/

	particles.clear();
	if (isFluidMode_)
	{
		for (float i = eps; i < 600 - eps * 2; i++)
		{
			for (float j = 600 / 15; j <= 600 / 2; j++)
			{
				if (particles.size() >= particleIndex)
				{

				}
				else
				{
					float jit = static_cast<float>(6 * rand()) / static_cast<float>(RAND_MAX);
					particles.emplace_back(Particle((j)+jit, i));
				}


				j += h_;
			}
			i += h_;
		}
	}
	else
	{
		particles.emplace_back(Particle(0.0f, 0.0f));
	}

	for (int i = 0; i <= particles.size() - 1; i++)
	{
		vertexFlowResource_[i] = CreateBufferResource(device_, sizeof(VertexData) * 4);
		indexFlowResource_[i] = CreateBufferResource(device_, sizeof(uint32_t) * 6);
	}

	vertexFlowBufferView_.BufferLocation = vertexFlowResource_[0]->GetGPUVirtualAddress();
	vertexFlowBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	vertexFlowBufferView_.StrideInBytes = sizeof(VertexData);

	indexFlowBufferView_.BufferLocation = indexFlowResource_[0]->GetGPUVirtualAddress();
	indexFlowBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexFlowBufferView_.Format = DXGI_FORMAT_R32_UINT;

	for (int i = 0; i <= particles.size() - 1; i++)
	{
		vertexFlowDate_[i] = nullptr;
		vertexFlowResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&vertexFlowDate_[i]));

		vertexFlowDate_[i][0] = { { -6.0f,6.0f,0.0f,1.0f }, { 0.0f,1.0f }, { 0.0f,0.0f,-1.0f } };
		vertexFlowDate_[i][1] = { { -6.0f,-6.0f,0.0f,1.0f }, { 0.0f,0.0f }, { 0.0f,0.0f,-1.0f } };
		vertexFlowDate_[i][2] = { { 6.0f,6.0f,0.0f,1.0f }, { 1.0f,1.0f }, { 0.0f,0.0f,-1.0f } };
		vertexFlowDate_[i][3] = { { 6.0f,-6.0f,0.0f,1.0f }, { 1.0f,0.0f }, { 0.0f,0.0f,-1.0f } };


		indexFlowResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&indexFlowData_[i]));
		indexFlowData_[i][0] = 0;
		indexFlowData_[i][1] = 1;
		indexFlowData_[i][2] = 2;

		indexFlowData_[i][3] = 1;
		indexFlowData_[i][4] = 3;
		indexFlowData_[i][5] = 2;


		wvpFlowResource_[i] = CreateBufferResource(device_, sizeof(TransformationMatrix));
		wvpFlowDate_[i] = nullptr;
		wvpFlowResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpFlowDate_[i]));
		wvpFlowDate_[i]->WVP = MakeIdentity4x4();
		wvpFlowDate_[i]->World = MakeIdentity4x4();
	}
	materialFlowResource_ = CreateBufferResource(device_, sizeof(Material));
	materialFlowDate_ = nullptr;
	materialFlowResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialFlowDate_));
	materialFlowDate_->color = particleColor;
	materialFlowDate_->enableLighting = false;
	materialFlowDate_->uvTransform = MakeIdentity4x4();

	particleDateResource_ = CreateBufferResource(device_, sizeof(ParticleDate));
	particleDate_ = nullptr;
	particleDateResource_->Map(0, nullptr, reinterpret_cast<void**>(&particleDate_));
	particleDate_->particleCount = int(particles.size() - 1);
	particleDate_->threshold = 1.6f;
	for (int i = 0; i < particleDate_->particleCount; i++)
	{
		particleDate_->center[i] = { 0.0f,0.0f,0.0f,0.0f };
		particleDate_->radius[i] = { 6.0f,0.0f,0.0f,0.0f };
	}

	particleDate_->padding[0] = 0;
	particleDate_->padding[1] = 0;

	Matrix4x4 viewMatSprite = MakeIdentity4x4();
	Matrix4x4 projectMatSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(Fuji::GetkWindowWidth()), float(Fuji::GetkWindowHeight()), 0.0f, 100.0f);
	Matrix4x4 worldViewProMatSpriteOrigine = Multiply(viewMatSprite, projectMatSprite);
	Matrix4x4 worldViewProMatSprite = worldViewProMatSpriteOrigine;
	for (size_t i = 0; i <= particles.size() - 1; i++)
	{
		Matrix4x4 worldMatSprite = MakeAffineMatrix(Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), particles[i].pos);
		worldViewProMatSprite = worldViewProMatSpriteOrigine;
		worldViewProMatSprite = Multiply(worldMatSprite, worldViewProMatSprite);

		wvpFlowDate_[i]->World = worldMatSprite;
		wvpFlowDate_[i]->WVP = worldViewProMatSprite;

		particleDate_->center[i] = { worldMatSprite.m[3][0],worldMatSprite.m[3][1],0.0f,0.0f };
	}
}

void DXCom::SettingSpriteVertex()
{
	/*vertexResourceSprite_ = CreateBufferResource(device_.Get(), sizeof(VertexData) * 4);

	vertexBufferViewSprite_.BufferLocation = vertexResourceSprite_->GetGPUVirtualAddress();
	vertexBufferViewSprite_.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferViewSprite_.StrideInBytes = sizeof(VertexData);

	vertexDataSprite_ = nullptr;
	vertexResourceSprite_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite_));

	vertexDataSprite_[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexDataSprite_[0].texcoord = { 0.0f,1.0f };
	vertexDataSprite_[0].normal = { 0.0f,0.0f,-1.0f };

	vertexDataSprite_[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite_[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite_[1].normal = { 0.0f,0.0f,-1.0f };

	vertexDataSprite_[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite_[2].texcoord = { 1.0f,1.0f };
	vertexDataSprite_[2].normal = { 0.0f,0.0f,-1.0f };


	vertexDataSprite_[3].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite_[3].texcoord = { 1.0f,0.0f };
	vertexDataSprite_[3].normal = { 0.0f,0.0f,-1.0f };


	indexResoureceSprite = CreateBufferResource(device_.Get(), sizeof(uint32_t) * 6);
	indexBufferViewSprite.BufferLocation = indexResoureceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	indexResoureceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;

	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;*/

	////model
	//modelData_ = LoadObjFile("resource", "axis.obj");
	//vertexModelResource_ = CreateBufferResource(device_.Get(), sizeof(VertexData) * modelData_.vertices.size());
	//vertexModelBufferView_.BufferLocation = vertexModelResource_->GetGPUVirtualAddress();
	//vertexModelBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());
	//vertexModelBufferView_.StrideInBytes = sizeof(VertexData);

	//vertexModelResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel_));
	//std::memcpy(vertexDataModel_, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());

	//wvpResourceModel_ = CreateBufferResource(device_.Get(), sizeof(TransformationMatrix));
	//wvpDateModel_ = nullptr;
	//wvpResourceModel_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDateModel_));
	//wvpDateModel_->WVP = MakeIdentity4x4();
	//wvpDateModel_->World = MakeIdentity4x4();

	//materialResourceModel_ = CreateBufferResource(device_.Get(), sizeof(Material));
	//materialDateModel_ = nullptr;
	//materialResourceModel_->Map(0, nullptr, reinterpret_cast<void**>(&materialDateModel_));
	////色変えるやつ（Resource）
	//materialDateModel_->color = { 1.0f,1.0f,1.0f,1.0f };
	//materialDateModel_->enableLighting = false;
	//materialDateModel_->uvTransform = MakeIdentity4x4();

	/*directionalLightResourceModel_ = CreateBufferResource(device_.Get(), sizeof(DirectionalLight));
	directionalLightDataModel_ = nullptr;
	directionalLightResourceModel_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataModel_));
	directionalLightDataModel_->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightDataModel_->direction = { 1.0f,0.0f,0.0f };
	directionalLightDataModel_->intensity = 1.0f;*/


	//FenceModel
	fenceModelData_ = LoadObjFile("resource", "fence.obj");
	vertexFenceModelResource_ = CreateBufferResource(device_.Get(), sizeof(VertexData) * fenceModelData_.vertices.size());
	vertexFenceModelBufferView_.BufferLocation = vertexFenceModelResource_->GetGPUVirtualAddress();
	vertexFenceModelBufferView_.SizeInBytes = UINT(sizeof(VertexData) * fenceModelData_.vertices.size());
	vertexFenceModelBufferView_.StrideInBytes = sizeof(VertexData);

	vertexFenceModelResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataFenceModel_));
	std::memcpy(vertexDataFenceModel_, fenceModelData_.vertices.data(), sizeof(VertexData) * fenceModelData_.vertices.size());

	wvpResourceFenceModel_ = CreateBufferResource(device_.Get(), sizeof(TransformationMatrix));
	wvpDateFenceModel_ = nullptr;
	wvpResourceFenceModel_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDateFenceModel_));
	wvpDateFenceModel_->WVP = MakeIdentity4x4();
	wvpDateFenceModel_->World = MakeIdentity4x4();

	materialResourceFenceModel_ = CreateBufferResource(device_.Get(), sizeof(Material));
	materialDateFenceModel_ = nullptr;
	materialResourceFenceModel_->Map(0, nullptr, reinterpret_cast<void**>(&materialDateFenceModel_));
	//色変えるやつ（Resource）
	materialDateFenceModel_->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDateFenceModel_->enableLighting = false;
	materialDateFenceModel_->uvTransform = MakeIdentity4x4();

	directionalLightResourceFenceModel_ = CreateBufferResource(device_.Get(), sizeof(DirectionalLight));
	directionalLightDataFenceModel_ = nullptr;
	directionalLightResourceFenceModel_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataFenceModel_));
	directionalLightDataFenceModel_->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightDataFenceModel_->direction = { 1.0f,0.0f,0.0f };
	directionalLightDataFenceModel_->intensity = 1.0f;

}

void DXCom::SettingResource()
{
	wvpResource_ = CreateBufferResource(device_.Get(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();

	materialResource_ = CreateBufferResource(device_.Get(), sizeof(Material));
	materialDate_ = nullptr;
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialDate_));
	//色変えるやつ（Resource）
	materialDate_->color = { 1.0f,1.0f,1.0f,1.0f };
	materialDate_->enableLighting = true;
	materialDate_->uvTransform = MakeIdentity4x4();

	directionalLightResource_ = CreateBufferResource(device_.Get(), sizeof(DirectionalLight));
	directionalLightData_ = nullptr;
	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
	directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData_->direction = { 1.0f,0.0f,0.0f };
	directionalLightData_->intensity = 1.0f;


	//wvpResource2_ = CreateBufferResource(device_.Get(), sizeof(TransformationMatrix));
	//wvpDate2_ = nullptr;
	//wvpResource2_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate2_));
	//wvpDate2_->WVP = MakeIdentity4x4();
	//wvpDate2_->World = MakeIdentity4x4();

	//materialResource2_ = CreateBufferResource(device_.Get(), sizeof(Material));
	//materialDate2_ = nullptr;
	//materialResource2_->Map(0, nullptr, reinterpret_cast<void**>(&materialDate2_));
	////色変えるやつ（Resource）
	//materialDate2_->color = { 1.0f,1.0f,1.0f,1.0f };
	//materialDate2_->enableLighting = false;
	//materialDate2_->uvTransform = MakeIdentity4x4();

	//directionalLightResource2_ = CreateBufferResource(device_.Get(), sizeof(DirectionalLight));
	//directionalLightData2_ = nullptr;
	//directionalLightResource2_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData2_));
	//directionalLightData2_->color = { 1.0f,1.0f,1.0f,1.0f };
	//directionalLightData2_->direction = { 1.0f,0.0f,0.0f };
	//directionalLightData2_->intensity = 1.0f;

}

void DXCom::SettingSpriteResource()
{
	//transformationMatResourceSprite_ = CreateBufferResource(device_.Get(), sizeof(TransformationMatrix));
	//transformationMatDataSprite_ = nullptr;
	//transformationMatResourceSprite_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatDataSprite_));
	//transformationMatDataSprite_->WVP = MakeIdentity4x4();
	//transformationMatDataSprite_->World = MakeIdentity4x4();


	//materialResourceSprite_ = CreateBufferResource(device_.Get(), sizeof(Material));
	//materialDateSprite_ = nullptr;
	//materialResourceSprite_->Map(0, nullptr, reinterpret_cast<void**>(&materialDateSprite_));
	////色変えるやつ（Resource）
	//materialDateSprite_->color = { 1.0f,1.0f,1.0f,1.0f };
	//materialDateSprite_->enableLighting = false;
	//materialDateSprite_->uvTransform = MakeIdentity4x4();
}

void DXCom::SettingTexture()
{
	const uint32_t descriptorSizeSRV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	mipImages_ = LoadTexture("resource/boal16x16.png");
	const DirectX::TexMetadata& metadata = mipImages_.GetMetadata();
	textureResource_ = CreateTextureResource(device_.Get(), metadata);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	intermediateResource1 = UploadTextureData(textureResource_, mipImages_, device_.Get(), commandList_);
	textureSrvHandleCPU = GetCPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 2);
	textureSrvHandleGPU = GetGPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 2);

	device_->CreateShaderResourceView(textureResource_.Get(), &srvDesc, textureSrvHandleCPU);



	/*mipImages2_ = LoadTexture(modelData_.material.textureFilePath);
	const DirectX::TexMetadata& metadata2 = mipImages2_.GetMetadata();
	textureResource2_ = CreateTextureResource(device_.Get(), metadata2);*/

	/*D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);*/

	/*intermediateResource2 = UploadTextureData(textureResource2_, mipImages2_, device_.Get(), commandList_);
	textureSrvHandleCPU2 = GetCPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 3);
	textureSrvHandleGPU2 = GetGPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 3);

	device_->CreateShaderResourceView(textureResource2_.Get(), &srvDesc2, textureSrvHandleCPU2);*/



	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescOff{};
	srvDescOff.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDescOff.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescOff.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescOff.Texture2D.MipLevels = 1;
	offTextureHandleCPU_ = GetCPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 3);
	offTextureHandle_ = GetGPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 3);
	device_->CreateShaderResourceView(offscreenrt_.Get(), &srvDescOff, offTextureHandleCPU_);



	//FenceModelSpriteData
	fenceMipImages_ = LoadTexture(fenceModelData_.material.textureFilePath);
	const DirectX::TexMetadata& fenceMetadata = fenceMipImages_.GetMetadata();
	fenceTextureResource_ = CreateTextureResource(device_.Get(), fenceMetadata);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescFence{};
	srvDescFence.Format = fenceMetadata.format;
	srvDescFence.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescFence.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescFence.Texture2D.MipLevels = UINT(fenceMetadata.mipLevels);

	fenceIntermediateResource_ = UploadTextureData(fenceTextureResource_, fenceMipImages_, device_.Get(), commandList_);
	fenceTextureSrvHandleCPU_ = GetCPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 4);
	fenceTextureSrvHandleGPU_ = GetGPUDescriptorHandle(ImGuiManager::GetInstance()->GetsrvHeap(), descriptorSizeSRV, 4);

	device_->CreateShaderResourceView(fenceTextureResource_.Get(), &srvDescFence, fenceTextureSrvHandleCPU_);



	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists->GetAddressOf());

	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_)
	{
		HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(fenceEvent != nullptr);
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}

	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));

}


void DXCom::FirstFrame()
{

}

void DXCom::SetBarrier()
{

	/*UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);*/



	D3D12_RESOURCE_BARRIER offbarrier{};
	offbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	offbarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	offbarrier.Transition.pResource = offscreenrt_.Get();
	offbarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	offbarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &offbarrier);


}

void DXCom::ClearRTV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	//UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	//commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);
	//commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	/*float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);*/

	commandList_->OMSetRenderTargets(1, &rtvHandles_[2], false, &dsvHandle);
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList_->ClearRenderTargetView(rtvHandles_[2], clearColorValue.Color, 0, nullptr);
}

void DXCom::Command()
{
	D3D12_VIEWPORT viewport{};
	viewport.Width = MyWin::kWindowWidth;
	viewport.Height = MyWin::kWindowHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = MyWin::kWindowWidth;
	scissorRect.top = 0;
	scissorRect.bottom = MyWin::kWindowHeight;


	commandList_->RSSetViewports(1, &viewport);
	commandList_->RSSetScissorRects(1, &scissorRect);
	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
	commandList_->SetPipelineState(graphicsPipelineState_.Get());

	//三角形１
	/*if (isTriangleDraw_)
	{

		commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
		commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
		commandList_->DrawInstanced(1536, 1, 0, 0);
	}*/


	////三角形２
	/*commandList_->RSSetViewports(1, &viewport);
	commandList_->RSSetScissorRects(1, &scissorRect);
	commandList_->SetGraphicsRootSignature(roootSignature_);
	commandList_->SetPipelineState(graphicsPipelineState_);
	commandList_->IASetVertexBuffers(0, 1, &vertexBufferView2_);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, materialResource2_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, wvpResource2_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource2_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, useMonsterBall2 ? textureSrvHandleGPU2 : textureSrvHandleGPU);
	commandList_->DrawInstanced(3, 1, 0, 0);*/

	//パーティクル
	/*for (int i = 0; i < particleIndex; i++)
	{
		if (isParticleLive[i])
		{
			commandList_->RSSetViewports(1, &viewport);
			commandList_->RSSetScissorRects(1, &scissorRect);
			commandList_->SetGraphicsRootSignature(roootSignature_);
			commandList_->SetPipelineState(graphicsPipelineState_);
			commandList_->IASetVertexBuffers(0, 1, &vertexParticleBufferView_[i]);
			commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList_->SetGraphicsRootConstantBufferView(0, materialParticleResource_[i]->GetGPUVirtualAddress());
			commandList_->SetGraphicsRootConstantBufferView(1, wvpParticleResource_[i]->GetGPUVirtualAddress());
			commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
			commandList_->DrawInstanced(3, 1, 0, 0);
		}
	}*/



	//model
	/*commandList_->RSSetViewports(1, &viewport);
	commandList_->RSSetScissorRects(1, &scissorRect);
	commandList_->SetGraphicsRootSignature(roootSignature_);
	commandList_->SetPipelineState(graphicsPipelineState_);
	commandList_->IASetVertexBuffers(0, 1, &vertexModelBufferView_);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, materialResourceModel_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, wvpResourceModel_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResourceModel_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);
	commandList_->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);*/



	//FenceModel
	commandList_->RSSetViewports(1, &viewport);
	commandList_->RSSetScissorRects(1, &scissorRect);
	commandList_->SetGraphicsRootSignature(rootSignature_.Get());
	commandList_->SetPipelineState(graphicsPipelineState_.Get());
	commandList_->IASetVertexBuffers(0, 1, &vertexFenceModelBufferView_);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, materialResourceFenceModel_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, wvpResourceFenceModel_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResourceFenceModel_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, fenceTextureSrvHandleGPU_);
	commandList_->DrawInstanced(UINT(fenceModelData_.vertices.size()), 1, 0, 0);



	/*commandList_->IASetIndexBuffer(&indexBufferViewSprite);
	commandList_->IASetVertexBuffers(0, 1, &vertexBufferViewSprite_);
	commandList_->SetGraphicsRootConstantBufferView(0, materialResourceSprite_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationMatResourceSprite_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);*/
	/*commandList_->DrawInstanced(6, 1, 0, 0);*/



	if (isFluidMode_)
	{
		for (int i = 0; i < particles.size() - 1; i++)
		{

			commandList_->IASetIndexBuffer(&indexFlowBufferView_);
			commandList_->IASetVertexBuffers(0, 1, &vertexFlowBufferView_);
			commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList_->SetGraphicsRootConstantBufferView(0, materialFlowResource_->GetGPUVirtualAddress());
			commandList_->SetGraphicsRootConstantBufferView(1, wvpFlowResource_[i]->GetGPUVirtualAddress());
			commandList_->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());
			commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
			commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);

			/*commandList_->DrawInstanced(3, 1, 0, 0);*/
		}
	}


}

void DXCom::PostEffect()
{
	D3D12_RESOURCE_BARRIER offbarrier{};
	offbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	offbarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	offbarrier.Transition.pResource = offscreenrt_.Get();
	offbarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	offbarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList_->ResourceBarrier(1, &offbarrier);


	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	D3D12_VIEWPORT viewport{};
	viewport.Width = MyWin::kWindowWidth;
	viewport.Height = MyWin::kWindowHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = MyWin::kWindowWidth;
	scissorRect.top = 0;
	scissorRect.bottom = MyWin::kWindowHeight;


	if (isGrayscale_)
	{
		commandList_->RSSetViewports(1, &viewport);
		commandList_->RSSetScissorRects(1, &scissorRect);
		commandList_->SetGraphicsRootSignature(rootSignatureGrayscale_.Get());
		commandList_->SetPipelineState(graphicsPipelineStateGary_.Get());

		commandList_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetIndexBuffer(&indexGrayBufferView_);
		commandList_->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		commandList_->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	if (isMetaBall_)
	{
		commandList_->RSSetViewports(1, &viewport);
		commandList_->RSSetScissorRects(1, &scissorRect);
		commandList_->SetGraphicsRootSignature(rootSignatureMetaBall_.Get());
		commandList_->SetPipelineState(graphicsPipelineStateMeta_.Get());

		commandList_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetIndexBuffer(&indexGrayBufferView_);
		commandList_->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		commandList_->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		commandList_->SetGraphicsRootConstantBufferView(1, particleDateResource_->GetGPUVirtualAddress());
		commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	if (isGaussian_)
	{
		commandList_->RSSetViewports(1, &viewport);
		commandList_->RSSetScissorRects(1, &scissorRect);
		commandList_->SetGraphicsRootSignature(rootSignatureGauss_.Get());
		commandList_->SetPipelineState(graphicsPipelineStateGauss_.Get());

		commandList_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetIndexBuffer(&indexGrayBufferView_);
		commandList_->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		commandList_->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	if (isNonePost_)
	{
		commandList_->RSSetViewports(1, &viewport);
		commandList_->RSSetScissorRects(1, &scissorRect);
		commandList_->SetGraphicsRootSignature(rootSignatureNone_.Get());
		commandList_->SetPipelineState(graphicsPipelineStateNone_.Get());

		commandList_->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetIndexBuffer(&indexGrayBufferView_);
		commandList_->IASetVertexBuffers(0, 1, &vertexGrayBufferView_);
		commandList_->SetGraphicsRootDescriptorTable(0, offTextureHandle_);
		commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

}

void DXCom::LastFrame()
{
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();




	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier);


	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));


	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists->GetAddressOf());
	swapChain_->Present(1, 0);

	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_)
	{
		HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(fenceEvent != nullptr);
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}

	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));



}

void DXCom::SetWVPData(const Matrix4x4& world, const Matrix4x4& wvp)
{
	wvpDate_->World = world;
	wvpDate_->WVP = wvp;
}

void DXCom::SetSpriteWVPData(const Matrix4x4& world, const Matrix4x4& wvp)
{
	world;
	wvp;
	/*transformationMatDataSprite_->World = world;
	transformationMatDataSprite_->WVP = wvp;*/
}

void DXCom::UpDate()
{
	/*Tick();*/

#ifdef _DEBUG
	ImGui::Begin("debug");
	/*if (ImGui::TreeNode("Triangle1"))
	{
		ImGui::Checkbox("drawTriangle", &isTriangleDraw_);
		ImGui::ColorEdit3("color", &materialDate_->color.X);
		ImGui::DragFloat3("trans", &transform.translate.x, 0.01f, -2.0f, 5.0f);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f, -4.0f, 4.0f);
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f, 0.0f, 6.0f);
		ImGui::Checkbox("whiteTexture or uvChecker", &useMonsterBall);
		ImGui::TreePop();
	}*/

	/*if (ImGui::TreeNode("Triangle2"))
	{
		ImGui::ColorEdit3("color2", &materialDate2_->color.X);
		ImGui::DragFloat3("trans2", &transform2.translate.x, 0.01f, -2.0f, 2.0f);
		ImGui::DragFloat3("rotate2", &transform2.rotate.x, 0.01f, -4.0f, 4.0f);
		ImGui::DragFloat3("scale2", &transform2.scale.x, 0.01f, 0.0f, 6.0f);
		ImGui::Checkbox("useMonsterBall2", &useMonsterBall2);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Sprite"))
	{
		ImGui::DragFloat3("Sprite trans", &transSprite.translate.x, 1.0f, -1280.0f, 1280.0f);
		ImGui::DragFloat3("Sprite rotate", &transSprite.rotate.x, 0.01f, -4.0f, 4.0f);
		ImGui::DragFloat3("Sprite sclae", &transSprite.scale.x, 0.01f, 0.0f, 6.0f);
		ImGui::DragFloat2("uvtrans", &uvTransSprite.translate.x, 0.1f, -1280.0f, 1280.0f);
		ImGui::DragFloat("uvrotate", &uvTransSprite.rotate.z, 0.01f, -4.0f, 4.0f);
		ImGui::DragFloat2("uvsclae", &uvTransSprite.scale.x, 0.01f, 0.0f, 6.0f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("PlaneModel"))
	{
		ImGui::ColorEdit3("Modelcolor", &materialDateModel_->color.X);
		ImGui::DragFloat3("Modeltrans", &transformModel.translate.x, 0.01f, -2.0f, 2.0f);
		ImGui::DragFloat3("Modelrotate", &transformModel.rotate.x, 0.01f, -4.0f, 4.0f);
		ImGui::DragFloat3("Modelscale", &transformModel.scale.x, 0.01f, 0.0f, 6.0f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Particle"))
	{
		ImGui::ColorEdit3("Particlecolor", &particleColor.X);
		ImGui::TreePop();
	}*/

	ImGui::Checkbox("Gray", &isGrayscale_);
	ImGui::Checkbox("None", &isNonePost_);
	ImGui::Checkbox("Meta", &isMetaBall_);
	ImGui::Checkbox("Blur", &isGaussian_);

	if (isFluidMode_)
	{
		ImGui::DragFloat("mass", &mass_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("h", &h_, 0.01f, 0.0f, 64.0f);
		h2_ = h_ * h_;
		poly6 = 4.0f / (mpi_ * powf(h_, 8));
		spikyGrad = -10.0f / (mpi_ * powf(h_, 5));
		viscLap = 40.0f / (mpi_ * powf(h_, 5));
		eps = h_;
		halfH_ = h_ * 0.5f;
		ImGui::DragFloat("dt", &dt, 0.00001f, 0.0001f, 0.001f);
	}
	else
	{
		ImGui::SliderFloat("Setradius", &setradius_, 3.0f, 100.0f);
	}

	if (ImGui::TreeNode("FenceModel"))
	{
		ImGui::ColorEdit3("Modelcolor", &materialDateFenceModel_->color.X);
		ImGui::DragFloat3("Modeltrans", &transformFenceModel_.translate.x, 0.01f, -2.0f, 2.0f);
		ImGui::DragFloat3("Modelrotate", &transformFenceModel_.rotate.x, 0.01f, -4.0f, 4.0f);
		ImGui::DragFloat3("Modelscale", &transformFenceModel_.scale.x, 0.01f, 0.0f, 6.0f);
		ImGui::TreePop();
	}

	ImGui::Text("light");
	ImGui::SliderFloat3("light color", &directionalLightData_->color.X, 0.0f, 1.0f);
	ImGui::SliderFloat3("light direction", &directionalLightData_->direction.x, -1.0f, 1.0f);
	ImGui::End();

#endif // _DEBUG

	directionalLightData_->direction = directionalLightData_->direction.Normalize();

	if (!isFluidMode_)
	{
		if (Input::GetInstance()->Input::IsTriggerMouse(0))
		{
			if (particleDate_->particleCount < 399)
			{
				Vector2 xy = Input::GetInstance()->Input::GetMousePosition();

				particles.emplace_back(Particle(xy.x, xy.y));
				particleDate_->particleCount = int(particles.size() - 1);
				int count = particleDate_->particleCount;


				vertexFlowResource_[count] = CreateBufferResource(device_, sizeof(VertexData) * 4);
				indexFlowResource_[count] = CreateBufferResource(device_, sizeof(uint32_t) * 6);


				vertexFlowDate_[count] = nullptr;
				vertexFlowResource_[count]->Map(0, nullptr, reinterpret_cast<void**>(&vertexFlowDate_[count]));
				vertexFlowDate_[count][0] = { { -6.0f,6.0f,0.0f,1.0f }, { 0.0f,1.0f }, { 0.0f,0.0f,-1.0f } };
				vertexFlowDate_[count][1] = { { -6.0f,-6.0f,0.0f,1.0f }, { 0.0f,0.0f }, { 0.0f,0.0f,-1.0f } };
				vertexFlowDate_[count][2] = { { 6.0f,6.0f,0.0f,1.0f }, { 1.0f,1.0f }, { 0.0f,0.0f,-1.0f } };
				vertexFlowDate_[count][3] = { { 6.0f,-6.0f,0.0f,1.0f }, { 1.0f,0.0f }, { 0.0f,0.0f,-1.0f } };


				indexFlowResource_[count]->Map(0, nullptr, reinterpret_cast<void**>(&indexFlowData_[count]));
				indexFlowData_[count][0] = 0;
				indexFlowData_[count][1] = 1;
				indexFlowData_[count][2] = 2;

				indexFlowData_[count][3] = 1;
				indexFlowData_[count][4] = 3;
				indexFlowData_[count][5] = 2;


				wvpFlowResource_[count] = CreateBufferResource(device_, sizeof(TransformationMatrix));
				wvpFlowDate_[count] = nullptr;
				wvpFlowResource_[count]->Map(0, nullptr, reinterpret_cast<void**>(&wvpFlowDate_[count]));
				particleDate_->radius[count] = { setradius_,0.0f,0.0f,0.0f };


				Matrix4x4 viewMatSprite = MakeIdentity4x4();
				Matrix4x4 projectMatSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(Fuji::GetkWindowWidth()), float(Fuji::GetkWindowHeight()), 0.0f, 100.0f);
				Matrix4x4 worldViewProMatSpriteOrigine = Multiply(viewMatSprite, projectMatSprite);
				Matrix4x4 worldViewProMatSprite = worldViewProMatSpriteOrigine;

				Matrix4x4 worldMatSprite = MakeAffineMatrix(Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), particles[count].pos);
				worldViewProMatSprite = worldViewProMatSpriteOrigine;
				worldViewProMatSprite = Multiply(worldMatSprite, worldViewProMatSprite);

				wvpFlowDate_[count]->World = worldMatSprite;
				wvpFlowDate_[count]->WVP = worldViewProMatSprite;

				particleDate_->center[count] = { worldMatSprite.m[3][0],worldMatSprite.m[3][1],0.0f,0.0f };
			}
		}
		if (Input::GetInstance()->Input::IsTriggerMouse(1))
		{
			if (particleDate_->particleCount > 0)
			{
				particles.pop_back();
				particleDate_->particleCount = int(particles.size() - 1);
			}
		}
	}
	else
	{
		for (auto& pi : particles)
		{
			pi.density = 0.00001f;
			for (auto& pj : particles)
			{
				Vector3 rij = pj.pos - pi.pos;
				Vector2 rijv2 = { rij.x,rij.y };
				float r2 = rijv2 * rijv2;

				if (r2 < h2_)
				{
					float c = h2_ - r2;
					pi.density += mass_ * poly6 * powf(c, 3);
				}
			}
			pi.pressure = gasConst * (pi.density - restDens);
		}
		for (auto& pi : particles)
		{
			Vector2 fpress(0, 0);
			Vector2 fvisc(0, 0);

			for (auto& pj : particles)
			{
				if (&pi == &pj)
				{
					continue;
				}
				Vector3 rij = pj.pos - pi.pos;
				Vector2 rijv2 = { rij.x,rij.y };
				float r = rijv2.Lenght();

				if (r < h_)
				{
					float c = h_ - r;

					fpress += (rijv2.NormaliZe() * (-1.0f)) * mass_ * (pi.pressure + pj.pressure) / (2.0f * pj.density) * spikyGrad * powf(c, 3);
					Vector2 pjv2Vel = { pj.vel.x,pj.vel.y };
					Vector2 piv2Vel = { pi.vel.x,pi.vel.y };
					fvisc += (pjv2Vel - piv2Vel) * visc * mass_ / pj.density * viscLap * c;
				}
			}
			Vector2 fgrav = G_ * mass_ / pi.density;
			Vector2 fff = (fpress + fvisc + fgrav);
			pi.force = { fff.x,fff.y,0.0f };
		}

		for (auto& p : particles)
		{
			p.vel += p.force * dt / p.density;
			p.pos += p.vel * dt;

			if (p.pos.x - eps < 0)
			{
				p.vel.x *= boundDamping;
				p.pos.x = eps;
			}
			if (p.pos.x + eps > 600)
			{
				p.vel.x *= boundDamping;
				p.pos.x = 600 - eps;
			}
			if (p.pos.y - eps < 0)
			{
				p.vel.y *= boundDamping;
				p.pos.y = eps;
			}
			if (p.pos.y + eps > 600)
			{
				p.vel.y *= boundDamping;
				p.pos.y = 600 - eps;
			}
		}

		gravityChangeTime_++;
		if (gravityChangeTime_ >= 0 && gravityChangeTime_ < 400)
		{
			G_ = { 0.0f,9.8f };
		}
		if (gravityChangeTime_ >= 400 && gravityChangeTime_ < 800)
		{
			G_ = { 0.0f,-9.8f };
		}
		if (gravityChangeTime_ >= 800 && gravityChangeTime_ < 1200)
		{
			G_ = { 9.8f,0.0f };
		}
		if (gravityChangeTime_ >= 1200 && gravityChangeTime_ < 1600)
		{
			G_ = { -9.8f,0.0f };
		}
		if (gravityChangeTime_ >= 1600)
		{
			gravityChangeTime_ = 0;
		}
	}



	/*transform.rotate.y += 0.05f;*/
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTrans.scale, cameraTrans.rotate, cameraTrans.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(Fuji::GetkWindowWidth()) / float(Fuji::GetkWindowHeight()), 0.1f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	wvpDate_->World = worldMatrix;
	wvpDate_->WVP = worldViewProjectionMatrix;


	Matrix4x4 worldMatrixFenceModel = MakeAffineMatrix(transformFenceModel_.scale, transformFenceModel_.rotate, transformFenceModel_.translate);
	Matrix4x4 worldViewProjectionMatrixFenceModel = Multiply(viewMatrix, projectionMatrix);
	worldViewProjectionMatrixFenceModel = Multiply(worldMatrixFenceModel, worldViewProjectionMatrixFenceModel);

	wvpDateFenceModel_->World = worldMatrixFenceModel;
	wvpDateFenceModel_->WVP = worldViewProjectionMatrixFenceModel;



	/*Matrix4x4 worldMatrix2 = MakeAffineMatrix(transform2.scale, transform2.rotate, transform2.translate);
	Matrix4x4 worldViewProjectionMatrix2 = Multiply(viewMatrix, projectionMatrix);
	worldViewProjectionMatrix2 = Multiply(worldMatrix2, worldViewProjectionMatrix2);

	wvpDate2_->World = worldMatrix2;
	wvpDate2_->WVP = worldViewProjectionMatrix2;

	Matrix4x4 worldMatSprite = MakeAffineMatrix(transSprite.scale, transSprite.rotate, transSprite.translate);
	Matrix4x4 viewMatSprite = MakeIdentity4x4();
	Matrix4x4 projectMatSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(Fuji::GetkWindowWidth()), float(Fuji::GetkWindowHeight()), 0.0f, 100.0f);
	Matrix4x4 worldViewProMatSprite = Multiply(viewMatSprite, projectMatSprite);
	worldViewProMatSprite = Multiply(worldMatSprite, worldViewProMatSprite);


	transformationMatDataSprite_->World = worldMatSprite;
	transformationMatDataSprite_->WVP = worldViewProMatSprite;

	Matrix4x4 uvtrasform = MakeScaleMatrix(uvTransSprite.scale);
	uvtrasform = Multiply(uvtrasform, MakeRotateZMatrix(uvTransSprite.rotate.z));
	uvtrasform = Multiply(uvtrasform, MakeTranslateMatrix(uvTransSprite.translate));
	materialDateSprite_->uvTransform = uvtrasform;


	Matrix4x4 worldMatrixModel = MakeAffineMatrix(transformModel.scale, transformModel.rotate, transformModel.translate);
	Matrix4x4 worldViewProjectionMatrixModel = Multiply(viewMatrix, projectionMatrix);
	worldViewProjectionMatrixModel = Multiply(worldMatrixModel, worldViewProjectionMatrixModel);

	wvpDateModel_->World = worldMatrixModel;
	wvpDateModel_->WVP = worldViewProjectionMatrixModel;*/

	/*std::mt19937 random(repopSeed());
	std::uniform_real_distribution<> number(-3.5, 3.5)
	//std::uniform_real_distribution<> velXZNumber(-0.05f, 0.05f);
	//std::uniform_real_distribution<> velYNumber(0.008f, 0.06f);
	//std::uniform_real_distribution<> rotNumber(-0.1f, 0.1f);
	std::uniform_real_distribution<> alphaRandom(0.4f, 1.0f);*/

	/*//for (int i = 0; i < particleIndex; i++)
	//{
	//	if (isParticleLive[i])
	//	{
	//		transformParticle[i].translate = transformParticle[i].translate + particleVel[i];
	//		transformParticle[i].rotate = transformParticle[i].rotate + particleRot[i];
	//		materialParticleDate_[i]->color.W -= 0.01f;
	//		if (materialParticleDate_[i]->color.W <= 0.0f)
	//		{
	//			materialParticleDate_[i]->color.W = 0.0f;
	//			isParticleLive[i] = false;
	//		}

	//		Matrix4x4 worldMatrixParticle = MakeAffineMatrix(transformParticle[i].scale, transformParticle[i].rotate, transformParticle[i].translate);
	//		Matrix4x4 worldViewProjectionMatrixParticle = Multiply(worldMatrixParticle, Multiply(viewMatrix, projectionMatrix));
	//		wvpParticleDate_[i]->World = worldMatrixParticle;
	//		wvpParticleDate_[i]->WVP = worldViewProjectionMatrixParticle;
	//	}
	//	else
	//	{

	//		float randomx = float(number(random));
	//		float randomz = float(number(random));

	//		transformParticle[i] = { {0.5f,0.5f,0.5f},{0.0f,0.0f,0.0f},{randomx,-2.6f,randomz} };

	//		//float randomvelx = float(velXZNumber(random));
	//		//float randomvely = float(velYNumber(random));
	//		//float randomvelz = float(velXZNumber(random));

	//		//float randomRotx = float(rotNumber(random));
	//		//float randomRoty = float(rotNumber(random));
	//		//float randomRotz = float(rotNumber(random));

	//		//particleVel[i] = { randomvelx,randomvely,randomvelz };
	//		//particleRot[i] = { randomRotx,randomRoty,randomRotz };

	//		materialParticleDate_[i]->color = particleColor;
	//		float alphacolor = float(alphaRandom(random));
	//		materialParticleDate_[i]->color.W = alphacolor;

	//		colorRandomAdd += 0.002f;
	//		if (colorRandomAdd>0.235f)
	//		{
	//			colorRandomAdd = 0;
	//		}
	//		materialParticleDate_[i]->color.Y += colorRandomAdd;

	//		Matrix4x4 worldMatrixParticle = MakeAffineMatrix(transformParticle[i].scale, transformParticle[i].rotate, transformParticle[i].translate);
	//		Matrix4x4 worldViewProjectionMatrixParticle = Multiply(worldMatrixParticle, Multiply(viewMatrix, projectionMatrix));
	//		wvpParticleDate_[i]->World = worldMatrixParticle;
	//		wvpParticleDate_[i]->WVP = worldViewProjectionMatrixParticle;
	//		isParticleLive[i] = true;


	//	}
	//}*/

	if (isFluidMode_)
	{
		Matrix4x4 viewMatSprite = MakeIdentity4x4();
		Matrix4x4 projectMatSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(Fuji::GetkWindowWidth()), float(Fuji::GetkWindowHeight()), 0.0f, 100.0f);
		Matrix4x4 worldViewProMatSpriteOrigine = Multiply(viewMatSprite, projectMatSprite);
		Matrix4x4 worldViewProMatSprite = worldViewProMatSpriteOrigine;
		for (size_t i = 0; i < particles.size() - 1; i++)
		{
			Matrix4x4 worldMatSprite = MakeAffineMatrix(Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), particles[i].pos);
			worldViewProMatSprite = worldViewProMatSpriteOrigine;
			worldViewProMatSprite = Multiply(worldMatSprite, worldViewProMatSprite);

			wvpFlowDate_[i]->World = worldMatSprite;
			wvpFlowDate_[i]->WVP = worldViewProMatSprite;

			particleDate_->center[i] = { worldMatSprite.m[3][0],worldMatSprite.m[3][1],0.0f,0.0f };

			/*Matrix4x4 worldMatrixParticle = MakeAffineMatrix(Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), particles[i].pos);
			Matrix4x4 worldViewProjectionMatrixParticle = Multiply(worldMatrixParticle, Multiply(viewMatrix, projectionMatrix));
			wvpFlowDate_[i]->World = worldMatrixParticle;
			wvpFlowDate_[i]->WVP = worldViewProjectionMatrixParticle;*/
		}
	}
}

void DXCom::ReleaseData()
{
	/*for (int i = 0; i < particleIndex; i++)
	{
		vertexParticleResource_[i]->Release();
		wvpParticleResource_[i]->Release();
		materialParticleResource_[i]->Release();
	}*/

	/*particles.clear();*/
	pixelShaderBlob_->Release();
	vertexShaderBlob_->Release();
	if (errorBlob_)
	{
		errorBlob_->Release();
	}
	signatureBlob_->Release();
	if (errorGrayBlob_)
	{
		errorGrayBlob_->Release();
	}
	if (signatureGrayBlob_)
	{
		signatureGrayBlob_->Release();
	}
	if (vertexShaderGrayBlob_)
	{
		vertexShaderGrayBlob_->Release();
	}
	if (pixelShaderGrayBlob_)
	{
		pixelShaderGrayBlob_->Release();
	}

#ifdef _DEBUG

#endif // _DEBUG


	CloseWindow(MyWin::GetInstance()->GetHwnd());
	CoUninitialize();

}






void DXCom::Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

std::wstring DXCom::ConvertString(const std::string& str)
{
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string DXCom::ConvertString(const std::wstring& str)
{
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

IDxcBlob* DXCom::CompileShader(const std::wstring& filePath, const wchar_t* profile, IDxcUtils* dxcUtils, IDxcCompiler3* dxcCompiler, IDxcIncludeHandler* includeHandler)
{
	Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;


	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};

	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));


	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Log(shaderError->GetStringPointer());
		assert(false);
	}


	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
	shaderSource->Release();
	shaderResult->Release();
	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes)
{
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferResourceDesc{};
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = sizeInBytes;
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;

	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr;
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DXCom::CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateOffscreenTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	//resourceDesc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, &color,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE DXCom::GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DXCom::GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

DirectX::ScratchImage DXCom::LoadTexture(const std::string& filePath)
{
	DirectX::ScratchImage image{};
	std::wstring filePathw = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathw.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));


	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;


	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

void DXCom::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages)
{
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; mipLevel++)
	{
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		HRESULT hr = texture->WriteToSubresource(UINT(mipLevel), nullptr, img->pixels,
			UINT(img->rowPitch), UINT(img->slicePitch));
		assert(SUCCEEDED(hr));
	}
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(),
		subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}

ModelData DXCom::LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	ModelData modeldata;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcords;
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		if (identifier == "v")
		{
			Vector4 position;
			s >> position.X >> position.Y >> position.Z;
			position.W = 1.0f;

			positions.push_back(position);
		}
		else if (identifier == "vt")
		{
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcords.push_back(texcoord);
		}
		else if (identifier == "vn")
		{
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (identifier == "f")
		{
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++)
			{
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのindexは　位置/uv/法線　で格納されているので、分解してindex取得
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; element++)
				{
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}
				Vector4 position = positions[elementIndices[0] - 1];
				position.X *= -1.0f;
				Vector2 texcoord = texcords[elementIndices[1] - 1];
				texcoord.y = 1.0f - texcoord.y;
				Vector3 normal = normals[elementIndices[2] - 1];
				normal.x *= -1.0f;
				triangle[faceVertex] = { position,texcoord,normal };
				/*VertexData vertex= { position,texcoord,normal };
				modeldata.vertices.push_back(vertex);*/
			}
			modeldata.vertices.push_back(triangle[2]);
			modeldata.vertices.push_back(triangle[1]);
			modeldata.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib")
		{
			std::string materialFilename;
			s >> materialFilename;
			modeldata.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modeldata;
}

MaterialData DXCom::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd")
		{
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	return materialData;
}

//void DXCom::Tick()
//{
//	auto currentTime = std::chrono::high_resolution_clock::now();
//	deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
//	lastTime = currentTime;
//
//	elapsedTime += deltaTime;
//	frameCount++;
//
//	if (elapsedTime >= 1.0f) {
//		frameRate = frameCount;
//		frameCount = 0;
//		elapsedTime = 0;
//	}
//}
