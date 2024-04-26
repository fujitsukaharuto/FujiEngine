#include "My.h"
#include <string>
#include <vector>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <dxcapi.h>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

#include "MatrixCalculation.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")


struct Vector4
{
	float X;
	float Y;
	float Z;
	float W;
};

struct Trans
{
	MyVec3 scale;
	MyVec3 rotate;
	MyVec3 translate;
};

struct VertexData
{
	Vector4 position;
	MyVec2 texcoord;
};

struct Sphere
{
	MyVec3 center;
	float radius;
};

void Log(const std::string& message);
std::wstring ConvertString(const std::string& str);
std::string ConvertString(const std::wstring& str);

IDxcBlob* CompileShader(const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler);

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

ID3D12DescriptorHeap* CreateDescriptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible
);

DirectX::ScratchImage LoadTexture(const std::string& filePath);

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages,
	ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

bool DepthFunc(float currZ, float prevZ);

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	CoInitializeEx(0, COINIT_MULTITHREADED);

#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(true);
	}
#endif // _DEBUG


	My::CreatWind();


	//DXGIファクトリーの作成
	IDXGIFactory7* dxgiFactory = nullptr;
	//HRESULTはWindows系のエラーコードで、関数が成功したかどうかをマクロで判断できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	//初期化の根本的な部分でエラーが出た場合は間違えているか、どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));


	IDXGIAdapter4* useAdapter = nullptr;
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアタブの場合見つからなかったことに
	}
	assert(useAdapter != nullptr);


	ID3D12Device* device = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr))
		{
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device != nullptr);
	Log("Complete creat D3D12Device!!\n");

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
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


	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));
	assert(SUCCEEDED(hr));


	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(hr));

	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(hr));


	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = MyWin::kWindowWidth;
	swapChainDesc.Height = MyWin::kWindowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, mywin_->GetHwnd(), &swapChainDesc,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));


	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);


	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));


	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);


	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, MyWin::kWindowWidth, MyWin::kWindowHeight);

	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);


	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));



	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
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


	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}


	ID3D12RootSignature* roootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&roootSignature));
	assert(SUCCEEDED(hr));

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl",
		L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl",
		L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);


	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;


	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	graphicPipelineStateDesc.pRootSignature = roootSignature;
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };

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

	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));




	ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * 1536);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 1536;
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDate = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexDate));


	const float pi = 3.1415926535f;
	const uint32_t kSubdivision = 16;
	const float kLonEvery = (pi * 2.0f) / static_cast<float>(kSubdivision);
	const float kLatEvery = (pi) / static_cast<float>(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	{
		float lat = pi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
		{
			float lon = lonIndex * kLonEvery;
			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;

			float u = float(lonIndex) / float(kSubdivision);
			float v = float(latIndex) / float(kSubdivision);

			vertexDate[startIndex].position.X = cosf(lat) * cosf(lon);
			vertexDate[startIndex].position.Y = sinf(lat);
			vertexDate[startIndex].position.Z = cosf(lat) * sinf(lon);
			vertexDate[startIndex].position.W = 1.0f;
			vertexDate[startIndex].texcoord = { u,v };

			u = float(lonIndex) / float(kSubdivision);
			v = float(latIndex+1) / float(kSubdivision);

			vertexDate[startIndex+1].position.X = cosf(lat+kLatEvery) * cosf(lon);
			vertexDate[startIndex+1].position.Y = sinf(lat+kLatEvery);
			vertexDate[startIndex+1].position.Z = cosf(lat+kLatEvery) * sinf(lon);
			vertexDate[startIndex+1].position.W = 1.0f;
			vertexDate[startIndex+1].texcoord = { u,v };

			u = float(lonIndex+1) / float(kSubdivision);
			v = float(latIndex) / float(kSubdivision);

			vertexDate[startIndex+2].position.X = cosf(lat) * cosf(lon+kLonEvery);
			vertexDate[startIndex+2].position.Y = sinf(lat);
			vertexDate[startIndex+2].position.Z = cosf(lat) * sinf(lon+kLonEvery);
			vertexDate[startIndex+2].position.W = 1.0f;
			vertexDate[startIndex+2].texcoord = { u,v };

			u = float(lonIndex) / float(kSubdivision);
			v = float(latIndex + 1) / float(kSubdivision);

			vertexDate[startIndex+3].position.X = cosf(lat+kLatEvery) * cosf(lon);
			vertexDate[startIndex+3].position.Y = sinf(lat+kLatEvery);
			vertexDate[startIndex+3].position.Z = cosf(lat+kLatEvery) * sinf(lon);
			vertexDate[startIndex+3].position.W = 1.0f;
			vertexDate[startIndex+3].texcoord = { u,v };

			u = float(lonIndex+1) / float(kSubdivision);
			v = float(latIndex + 1) / float(kSubdivision);

			vertexDate[startIndex+4].position.X = cosf(lat+kLatEvery) * cosf(lon+kLonEvery);
			vertexDate[startIndex+4].position.Y = sinf(lat+kLatEvery);
			vertexDate[startIndex+4].position.Z = cosf(lat+kLatEvery) * sinf(lon+kLonEvery);
			vertexDate[startIndex+4].position.W = 1.0f;
			vertexDate[startIndex+4].texcoord = { u,v };

			u = float(lonIndex+1) / float(kSubdivision);
			v = float(latIndex) / float(kSubdivision);

			vertexDate[startIndex+5].position.X = cosf(lat) * cosf(lon+kLonEvery);
			vertexDate[startIndex+5].position.Y = sinf(lat);
			vertexDate[startIndex+5].position.Z = cosf(lat) * sinf(lon+kLonEvery);
			vertexDate[startIndex+5].position.W = 1.0f;
			vertexDate[startIndex+5].texcoord = { u,v };


		}
	}


	/*vertexDate[0].position = { -0.5f,-0.5f,0.0f,1.0f };
	vertexDate[0].texcoord = { 0.0f,1.0f };

	vertexDate[1].position = { 0.0f,0.5f,0.0f,1.0f };
	vertexDate[1].texcoord = { 0.5f,0.0f };

	vertexDate[2].position = { 0.5f,-0.5f,0.0f,1.0f };
	vertexDate[2].texcoord = { 1.0f,1.0f };

	vertexDate[3].position = { -0.5f,-0.5f,0.5f,1.0f };
	vertexDate[3].texcoord = { 0.0f,1.0f };

	vertexDate[4].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDate[4].texcoord = { 0.5f,0.0f };

	vertexDate[5].position = { 0.5f,-0.5f,-0.5f,1.0f };
	vertexDate[5].texcoord = { 1.0f,1.0f };*/

	ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };

	vertexDataSprite[3].position = { 0.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[3].texcoord = { 0.0f,0.0f };
	vertexDataSprite[4].position = { 640.0f,0.0f,0.0f,1.0f };
	vertexDataSprite[4].texcoord = { 1.0f,0.0f };
	vertexDataSprite[5].position = { 640.0f,360.0f,0.0f,1.0f };
	vertexDataSprite[5].texcoord = { 1.0f,1.0f };

	ID3D12Resource* transformationMatResourceSprite = CreateBufferResource(device, sizeof(Matrix4x4));
	Matrix4x4* transformationMatDataSprite = nullptr;
	transformationMatResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatDataSprite));
	*transformationMatDataSprite = MakeIdentity4x4();
	Trans transSprite{ {1.0f,1.0f,1.0f,},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };


	ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Vector4));
	Vector4* materialDate = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialDate));
	//色変えるやつ（Resource）
	*materialDate = Vector4{ 1.0f,1.0f,1.0f,1.0f };


	ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
	Matrix4x4* wvpDate = nullptr;
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate));
	*wvpDate = MakeIdentity4x4();

	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	DirectX::ScratchImage mipImages = LoadTexture("resource/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	DirectX::ScratchImage mipImages2 = LoadTexture("resource/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	ID3D12Resource* textureResource2 = CreateTextureResource(device, metadata2);
	ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);

	device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandleCPU);


	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);

	device->CreateShaderResourceView(textureResource2, &srvDesc2, textureSrvHandleCPU2);


	Trans transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Trans cameraTrans{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };


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


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(mywin_->GetHwnd());
	ImGui_ImplDX12_Init(device, swapChainDesc.BufferCount, rtvDesc.Format, srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	Sphere sphere = { {0.0f,0.0f,0.0f},1.0f };
	bool useMonsterBall = true;

	//ウィンドウのxボタンが押されるまでループ
	while (My::ProcessMessage() == 0)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("debug");
		ImGui::SliderFloat("trans x", &transform.translate.x, -1.0f, 1.0f);
		ImGui::SliderFloat("trans y", &transform.translate.y, -1.0f, 1.0f);
		ImGui::SliderFloat("trans z", &transform.translate.z, -1.0f, 1.0f);
		ImGui::SliderFloat("transSprite x", &transSprite.translate.x, 0.0f, 700.0f);
		ImGui::SliderFloat("transSprite y", &transSprite.translate.y, 0.0f, 500.0f);
		ImGui::SliderFloat("transSprite z", &transSprite.translate.z, -100.0f, 100.0f);
		ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		ImGui::End();

		transform.rotate.y += 0.03f;
		Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTrans.scale, cameraTrans.rotate, cameraTrans.translate);
		Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight), 0.1f, 100.0f);
		Matrix4x4 worldViewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
		worldViewProjectionMatrix = Multiply(worldMatrix, worldViewProjectionMatrix);

		*wvpDate = worldViewProjectionMatrix;


		Matrix4x4 worldMatSprite = MakeAffineMatrix(transSprite.scale, transSprite.rotate, transSprite.translate);
		Matrix4x4 viewMatSprite = MakeIdentity4x4();
		Matrix4x4 projectMatSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(MyWin::kWindowWidth), float(MyWin::kWindowHeight), 0.0f, 100.0f);
		Matrix4x4 worldViewProMatSprite = Multiply(viewMatSprite, projectMatSprite);
		worldViewProMatSprite = Multiply(worldMatSprite, worldViewProMatSprite);
		*transformationMatDataSprite = worldViewProMatSprite;


		




		ImGui::ShowDemoWindow();


		ImGui::Render();


		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChainResources[backBufferIndex];
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);


		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
		commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);


		ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);


		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->SetGraphicsRootSignature(roootSignature);
		commandList->SetPipelineState(graphicsPipelineState);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

		commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

		commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

		commandList->DrawInstanced(1536, 1, 0, 0);


		commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
		commandList->SetGraphicsRootConstantBufferView(1, transformationMatResourceSprite->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
		commandList->DrawInstanced(6, 1, 0, 0);


		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);


		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrier);

		hr = commandList->Close();
		assert(SUCCEEDED(hr));


		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(1, commandLists);
		swapChain->Present(1, 0);

		fenceValue++;
		commandQueue->Signal(fence, fenceValue);
		if (fence->GetCompletedValue() < fenceValue)
		{
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		hr = commandAllocator->Reset();
		assert(SUCCEEDED(hr));
		hr = commandList->Reset(commandAllocator, nullptr);
		assert(SUCCEEDED(hr));
	}

	//OutputDebugStringA("Hello,DirectX!\n");
	//Log(ConvertString(std::format(L"WSTRING{}\n", wstringValue)));



	CloseHandle(fenceEvent);
	fence->Release();
	rtvDescriptorHeap->Release();
	srvDescriptorHeap->Release();
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	depthStencilResource->Release();
	dsvDescriptorHeap->Release();
	vertexResource->Release();
	vertexResourceSprite->Release();
	transformationMatResourceSprite->Release();
	depthStencilResource->Release();
	materialResource->Release();
	textureResource->Release();
	intermediateResource->Release();
	textureResource2->Release();
	intermediateResource2->Release();
	wvpResource->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob)
	{
		errorBlob->Release();
	}
	roootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();


#ifdef _DEBUG
	debugController->Release();
#endif // _DEBUG

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CloseWindow(mywin_->GetHwnd());


	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
	{
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	CoUninitialize();
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対応して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
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

std::string ConvertString(const std::wstring& str) {
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

IDxcBlob* CompileShader(const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler) {

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

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes)
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
	ID3D12Resource* resource = nullptr;
	HRESULT hr;
	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath)
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

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
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


	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages)
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
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(),
		subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height)
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

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

bool DepthFunc(float currZ, float prevZ)
{
	return currZ <= prevZ;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}
