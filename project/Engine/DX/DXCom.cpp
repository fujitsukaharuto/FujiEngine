#include "DXCom.h"
#include "Logger.h"
#include "SRVManager.h"
#include "ImGuiManager.h"


#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")


void DXCom::Initialize(MyWin* myWin) {
	assert(myWin);
	myWin_ = myWin;
	pipeManager_ = PipelineManager::GetInstance();
	pipeManager_->Initialize(this);
	offscreen_ = std::make_unique<OffscreenManager>();
	offscreen_->Initialize(this);

	CreateDevice();
	CreateCommand();
	CreateSwapChain();
	CreateRenderTargets();
	CreateDepthBuffer();


	CreateCompiler();
	InitializeFPSKeeper();
	SettingRootSignature();
	SettingGraphicPipeline();

	//SettingTexture();
	//SettingImgui();

}

void DXCom::Finalize() {
	if (pipeManager_) {
		pipeManager_->Finalize();
		pipeManager_ = nullptr;
	}
	if (offscreen_) {
		offscreen_.reset();
	}
	if (command_) {
		command_.reset();
	}
	compiler_.reset();
	depthStencilResource_.Reset();
	dsvDescriptorHeap_.Reset();
	for (auto& res : swapChainResources_) {
		res.Reset();
	}
	swapChain_.Reset();
	rtvDescriptorHeap_.Reset();
	device_.Reset();
	useAdapter_.Reset();
	dxgiFactory_.Reset();
#ifdef _DEBUG
	debugController_.Reset();
#endif
}

void DXCom::CreateDevice() {

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
	Logger::Log("Complete creat D3D12Device!!\n");

#ifdef _DEBUG
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

void DXCom::CreateCommand() {

	command_.reset(new DXCommand());
	command_->Initialize(device_.Get());

}

void DXCom::CreateSwapChain() {
	swapChain_ = nullptr;
	swapChainDesc_.Width = MyWin::kWindowWidth;
	swapChainDesc_.Height = MyWin::kWindowHeight;
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc_.SampleDesc.Count = 1;
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc_.BufferCount = 2;
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		command_->GetQueue(), myWin_->GetHwnd(), &swapChainDesc_,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));


}

void DXCom::CreateRenderTargets() {

	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);

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

	offscreen_->CreateResource();

}

void DXCom::CreateDepthBuffer() {

	depthStencilResource_ = CreateDepthStencilTextureResource(
		device_.Get(), MyWin::kWindowWidth, MyWin::kWindowHeight);

	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	dsvDesc_.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc_.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc_, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DXCom::CreateCompiler() {

	compiler_.reset(new DXCompil());
	compiler_->Initialize();

}

void DXCom::InitializeFPSKeeper() {
	fpsKeeper_ = FPSKeeper::GetInstance();
}


void DXCom::SettingRootSignature() {

	pipeManager_->CreatePipeline();


}

void DXCom::SettingGraphicPipeline() {

	/*for (uint32_t index = 0; index < instanceCount_; ++index)
	{
		particles_[index].transform.scale = { 1.0f,1.0f,1.0f };
		particles_[index].transform.rotate = { 0.0f,0.0f,0.0f };
		particles_[index].transform.translate = { index * 0.1f,index * 0.1f,index * 0.1f };

		particles_[index].velocity = { 0.0f,1.0f,0.0f };
	}*/

}

void DXCom::CreateBarrier(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[swapChain_->GetCurrentBackBufferIndex()].Get();
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	command_->GetList()->ResourceBarrier(1, &barrier);
}




void DXCom::SettingTexture() {
	//const uint32_t descriptorSizeSRV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	depthTextureSrvDesc_.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthTextureSrvDesc_.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthTextureSrvDesc_.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthTextureSrvDesc_.Texture2D.MipLevels = 1;
	uint32_t depthTexIndex = SRVManager::GetInstance()->Allocate();
	depthTexSrvHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(depthTexIndex);
	depthTexSrvHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(depthTexIndex);
	device_->CreateShaderResourceView(depthStencilResource_.Get(), &depthTextureSrvDesc_, depthTexSrvHandle_.first);

	offscreen_->SettingTexture();

	CommandExecution();
}


void DXCom::PreDraw() {
	TransitionResource(offscreen_->GetOffscreenResource().Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SRVManager::GetInstance()->SetDescriptorHeap();

	SetRenderTargets();
	ClearRenderTarget();
	ClearDepthBuffer();
}

void DXCom::Command() {


	command_->SetViewAndscissor();
	pipeManager_->SetPipeline(Pipe::Normal);


}

void DXCom::PostEffect() {

	ID3D12GraphicsCommandList* commandList = command_->GetList();

	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	CreateBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);
	/*commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);*/

	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	offscreen_->Command();
}

void DXCom::PostDraw() {


	CreateBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// 命令のクローズ
	command_->Close();
	// コマンドリストの実行
	swapChain_->Present(1, 0);
	command_->Execution();
	fpsKeeper_->FixFPS();
	command_->Reset();
}


void DXCom::PreSpriteDraw() {
	command_->SetViewAndscissor();
	pipeManager_->SetPipeline(Pipe::Normal);
	command_->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DXCom::CommandExecution() {
	command_->Close();

	command_->Execution();

	command_->Reset();
}

void DXCom::SetRenderTargets() {

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	command_->GetList()->OMSetRenderTargets(1, &rtvHandles_[2], false, &dsvHandle);
}

void DXCom::ClearRenderTarget() {

	command_->GetList()->ClearRenderTargetView(rtvHandles_[2], offscreen_->GetClearColorValue().Color, 0, nullptr);
}

void DXCom::ClearDepthBuffer() {

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	command_->GetList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DXCom::OffscreenUpDate() {
	offscreen_->Update();
}

void DXCom::OffscreenDebugGUI() {
	offscreen_->DebugGUI();
}


void DXCom::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	if (before == after) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	command_->GetList()->ResourceBarrier(1, &barrier);
}

void DXCom::InsertUAVBarrier(ID3D12Resource* resource) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = resource;

	command_->GetList()->ResourceBarrier(1, &barrier);
}

void DXCom::PreOutline() {
	TransitionResource(depthStencilResource_.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void DXCom::PostOutline() {
	TransitionResource(depthStencilResource_.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}


Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
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

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DXCom::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height) {
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

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateOffscreenTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color) {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags =
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

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

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateUAVResource(ID3D12Device* device, size_t sizeInBytes) {
	HRESULT hr;

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ComPtr<ID3D12Resource> bufferResource = nullptr;
	hr = device->CreateCommittedResource(
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&bufferResource));
	assert(SUCCEEDED(hr));

	return bufferResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateReadbackResource(ID3D12Device* device, size_t sizeInBytes) {
	HRESULT hr;

	// CPUから読み取れるヒープ設定
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource = nullptr;
	hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // CPUから読む用
		nullptr,
		IID_PPV_ARGS(&readbackResource)
	);
	assert(SUCCEEDED(hr));

	return readbackResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXCom::CreateUploadBuffer(size_t sizeInBytes, const void* initData) {
	HRESULT hr;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
	hr = device_->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	);
	assert(SUCCEEDED(hr));

	// 初期データがある場合はコピー
	if (initData) {
		void* mapped = nullptr;
		D3D12_RANGE range = { 0, 0 }; // 読み取りはしない
		hr = uploadBuffer->Map(0, &range, &mapped);
		assert(SUCCEEDED(hr));
		memcpy(mapped, initData, sizeInBytes);
		uploadBuffer->Unmap(0, nullptr);
	}

	return uploadBuffer;
}
