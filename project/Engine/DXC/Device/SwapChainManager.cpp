#include "Engine/DXC/Device/SwapChainManager.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include <cassert>
#include "Engine/Core/Time/FPSKeeper.h"

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
	// 第一引数、垂直同期は1がON、0がOFF。切るとモニターのリフレッシュレートを超えて描ける(負荷の余裕を測るとき用)
	if (Core::FPSKeeper::IsVSync()) {
		swapChain_->Present(1, 0);
	} else {
		// フラグが無いと、バッファ2枚ではモニターの更新を待たされて上限を超えない
		swapChain_->Present(0, isTearingSupported_ ? DXGI_PRESENT_ALLOW_TEARING : 0);
	}
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
	swapChainDesc.BufferCount = kBackBufferCount_;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	BOOL isTearingAllowed = FALSE;
	if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &isTearingAllowed, sizeof(isTearingAllowed)))) {
		isTearingSupported_ = (isTearingAllowed == TRUE);
	}
	swapChainDesc.Flags = isTearingSupported_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	// スワップチェーンの生成
	hr = factory->CreateSwapChainForHwnd(
		commandQueue, hwnd, &swapChainDesc,
		nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));
	width_ = width;
	height_ = height;

	// Alt+Enter は MyWin が枠なしフルスクリーンとして受ける。DXGI に任せると排他フルスクリーンへ入ろうとする
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
}

void SwapChainManager::Resize(ID3D12Device* device, int width, int height) {
	// 参照が残っていると ResizeBuffers が失敗する
	for (auto& res : swapChainResources_) {
		res.Reset();
	}
	// 作成時と同じフラグを渡さないと失敗する
	HRESULT hr = swapChain_->ResizeBuffers(kBackBufferCount_, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
		isTearingSupported_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
	assert(SUCCEEDED(hr));
	width_ = width;
	height_ = height;
	CreateBackBufferViews(device);
}

void SwapChainManager::CreateRenderTargets(ID3D12Device* device) {
	rtvDescriptorHeap_ = DXC::Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kRtvCount_, false);

	const D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	const UINT increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < kRtvCount_; ++i) {
		rtvHandles_[i].ptr = rtvStartHandle.ptr + static_cast<SIZE_T>(increment) * i;
	}

	CreateBackBufferViews(device);
}

void SwapChainManager::CreateBackBufferViews(ID3D12Device* device) {
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (UINT i = 0; i < kBackBufferCount_; ++i) {
		HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
		device->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}
}

void SwapChainManager::CreateDepthBuffer(ID3D12Device* device, int width, int height) {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		depthStencilResource_[i] = DXC::Helper::CreateDepthStencilTextureResource(
			device, width, height);
	}

	dsvDescriptorHeap_ = DXC::Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DXC::kFrameCount_, false);

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
