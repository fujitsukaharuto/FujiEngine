#include "Engine/DXC/Device/SwapChainManager.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include <cassert>

using namespace DXC;

void SwapChainManager::Initialize(ID3D12Device* device, IDXGIFactory7* factory, ID3D12CommandQueue* commandQueue, HWND hwnd, int width, int height) {
	// スワップチェーンの作成
	CreateSwapChain(factory, commandQueue, hwnd, width, height);
	// スワップチェーンのバックバッファを取得し、RTVを作成
	CreateRenderTargets(device);
	// 画面と同じサイズの深度バッファを作成
	CreateDepthBuffer(device, width, height);
}

void SwapChainManager::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		depthStencilResource_[i].Reset();
	}
	dsvDescriptorHeap_.Reset();
	for (auto& res : swapChainResources_) {
		res.Reset();
	}
	swapChain_.Reset();
	rtvDescriptorHeap_.Reset();
}

void SwapChainManager::Present() {
	swapChain_->Present(1, 0);// 第一引数、垂直同期は1がON、0がOFF
}

void SwapChainManager::CreateBarrier(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, DXCommand* command) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[swapChain_->GetCurrentBackBufferIndex()].Get();
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	command->GetList()->ResourceBarrier(1, &barrier);
}

void SwapChainManager::SettingTexture(ID3D12Device* device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc{};
	depthTextureSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthTextureSrvDesc.Texture2D.MipLevels = 1;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		uint32_t depthTexIndex = SRVManager::GetInstance()->Allocate();
		depthTexSrvHandle_[i].first = SRVManager::GetInstance()->GetCPUDescriptorHandle(depthTexIndex);
		depthTexSrvHandle_[i].second = SRVManager::GetInstance()->GetGPUDescriptorHandle(depthTexIndex);
		device->CreateShaderResourceView(depthStencilResource_[i].Get(), &depthTextureSrvDesc, depthTexSrvHandle_[i].first);
	}
}

void SwapChainManager::CreateSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* commandQueue, HWND hwnd, int width, int height) {
	HRESULT hr;
	swapChain_ = nullptr;

	// スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = backBufferCount_;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// スワップチェーンの生成
	hr = factory->CreateSwapChainForHwnd(
		commandQueue, hwnd, &swapChainDesc,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void SwapChainManager::CreateRenderTargets(ID3D12Device* device) {
	rtvDescriptorHeap_ = DXC::Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numRTVHandle_, false);

	HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	rtvHandles_[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc, rtvHandles_[0]);
	rtvHandles_[1].ptr = rtvHandles_[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc, rtvHandles_[1]);

	// ここより下はオフスクリーンで使う用
	rtvHandles_[2].ptr = rtvHandles_[1].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvHandles_[3].ptr = rtvHandles_[2].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	rtvHandles_[4].ptr = rtvHandles_[3].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvHandles_[5].ptr = rtvHandles_[4].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void SwapChainManager::CreateDepthBuffer(ID3D12Device* device, int width, int height) {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		depthStencilResource_[i] = DXC::Helper::CreateDepthStencilTextureResource(
			device, width, height);
	}

	dsvDescriptorHeap_ = DXC::Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2, false);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	UINT handleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {

		// 正しく CPU ハンドルをオフセット
		D3D12_CPU_DESCRIPTOR_HANDLE currentHandle = {};
		currentHandle.ptr = dsvHandle.ptr + i * handleIncrementSize;

		dsvHandle_[i] = dsvHandle;
		dsvHandle_[i].ptr += i * handleIncrementSize;

		device->CreateDepthStencilView(depthStencilResource_[i].Get(), &dsvDesc, currentHandle);
	}
}
