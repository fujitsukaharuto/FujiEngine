#pragma once
#include <Windows.h>   // HWND用
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <utility>
#include "Engine/DXC/FrameCount.h"

namespace DXC {
	class DXCommand;
}

namespace DXC {
	/// <summary>
	/// スワップチェーンとバックバッファのRTVを管理するクラス
	/// </summary>
	class SwapChainManager {
	public:
		SwapChainManager() = default;
		~SwapChainManager() = default;

		// 生成には Device, Factory, Windowハンドル, CommandQueue が必要
		void Initialize(ID3D12Device* device, IDXGIFactory7* factory, ID3D12CommandQueue* commandQueue, HWND hwnd, int width, int height);

		// 終了処理
		void Finalize();

		// 描画の開始と終了（Present）
		void Present();

		// スワップチェーン用のバリア変更
		void CreateBarrier(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, DXC::DXCommand* command);

		// 深度用Textureの初期化
		void SettingTexture(ID3D12Device* device);

		/// <summary>バックバッファをウィンドウの大きさに合わせて作り直す</summary>
		/// <remarks>GPU がバックバッファを使い終わってから呼ぶこと。ゲームを描く側の大きさは変わらない</remarks>
		void Resize(ID3D12Device* device, int width, int height);

		//========================================================================*/
		//* Getter
		IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }
		size_t GetBackBufferCount() const { return kBackBufferCount_; }
		int GetWidth() const { return width_; }
		int GetHeight() const { return height_; }
		ID3D12Resource* GetBackBuffer(UINT index) const { return swapChainResources_[index].Get(); }
		ID3D12Resource* GetDSVResource(UINT index) const { return depthStencilResource_[index].Get(); }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetBackBufferRTV(uint32_t index) const { return rtvHandles_[index]; }
		/// <summary>ゲームを描くオフスクリーンの RTV</summary>
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetOffscreenRTV(uint32_t frameIndex) const { return rtvHandles_[kOffscreenRtvStart_ + frameIndex]; }
		/// <summary>GPUパーティクルを描く 1/4 解像度の RTV</summary>
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetGPUParticleRTV(uint32_t frameIndex) const { return rtvHandles_[kGPUParticleRtvStart_ + frameIndex]; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSVHandle(uint32_t index) const { return dsvHandle_[index]; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetDepthTexGPUHandle(UINT index) { return depthTexSrvHandle_[index].second; }

	private:
		void CreateSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* commandQueue, HWND hwnd, int width, int height);
		void CreateRenderTargets(ID3D12Device* device);
		void CreateBackBufferViews(ID3D12Device* device);
		void CreateDepthBuffer(ID3D12Device* device, int width, int height);

	private:
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
		// 2枚だと垂直同期を切っても Present がモニターの更新を待つ
		static constexpr UINT kBackBufferCount_ = 3;
		// RTV ヒープの並び。バックバッファ、オフスクリーン、GPUパーティクルの順
		static constexpr UINT kOffscreenRtvStart_ = kBackBufferCount_;
		static constexpr UINT kGPUParticleRtvStart_ = kOffscreenRtvStart_ + DXC::kFrameCount_;
		static constexpr UINT kRtvCount_ = kGPUParticleRtvStart_ + DXC::kFrameCount_;

		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[kBackBufferCount_];
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kRtvCount_];
		// バックバッファの大きさ。ウィンドウの大きさに追従する
		int width_ = 0;
		int height_ = 0;
		// 垂直同期を切ったとき、モニターの更新を待たずに出せるか
		bool isTearingSupported_ = false;

		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_[DXC::kFrameCount_];
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> depthTexSrvHandle_[DXC::kFrameCount_];
	};
}