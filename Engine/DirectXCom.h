#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "MyWindow.h"

class DirectXCom
{
public:
	
	static DirectXCom* GetInstance();
	void Init(MyWin* win, int32_t backBufferWidth = MyWin::kWindowWidth,
		int32_t backBufferHeight = MyWin::kWindowHeight);
	void PreDraw();
	void PostDraw();
	void ClearRenderTarget();

private:
	MyWin* myWin_;
	int32_t backBufferWidth_ = 0;
	int32_t backBufferHeight_ = 0;

	IDXGISwapChain4* swapChain_;
	ID3D12Resource* swapChainResources_[2];
	ID3D12CommandQueue* commandQueue_;
	ID3D12GraphicsCommandList* commandList_;
	ID3D12CommandAllocator* commandAllocator_;
	IDXGIFactory7* dxgiFactory_;
	ID3D12Device* device_ = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
	D3D12_RESOURCE_BARRIER barrier{};
	ID3D12Fence* fence_;
	uint64_t fenceValue_;


	DirectXCom() = default;
	~DirectXCom() = default;
	DirectXCom(const DirectXCom&) = delete;

	const DirectXCom& operator=(const DirectXCom&) = delete;

	void InitDXGIDevice();
	void CreateSwapChain();
	void InitCommand();
	void CreatRenderTarget();
	void CreateFence();
};