
#pragma once
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <cassert>
#include <wrl.h>


#include "Math/Matrix/MatrixCalculation.h"
#include "MyWindow.h"
#include "DXCommand.h"
#include "DXCompil.h"
#include "OffscreenManager.h"

#include "FPSKeeper.h"
#include "PipelineManager.h"
#include "TextureManager.h"
#include "Camera.h"



const int particleIndex = 400;
const UINT instanceCount_ = 10;

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};



enum class BlendMode {
	// ブレンド無し
	kBlendModeNone,
	// 通常αブレンド
	kBlendModeNormal,
	// 加算
	kBlendModeAdd,
	// 現在
	kBlendModeSubtract,
	// 乗算
	kBlendModeMultiply,
	// スクリーン
	kBlendModeScreen,
};




class DebugCamera;//今だけ、後から消す


class DXCom {
public:
	DXCom() = default;
	~DXCom() = default;
public:

	void Initialize(MyWin* myWin);
	void Finalize();

	void SettingTexture();

	/// <summary>
	/// 描画開始時
	/// </summary>
	void PreDraw();
	void Command();

	/// <summary>
	/// ポストエフェクト
	/// </summary>
	void PostEffect();

	/// <summary>
	/// 描画終了
	/// </summary>
	void PostDraw();

	void PreSpriteDraw();

	/// <summary>
	/// 実行
	/// </summary>
	void CommandExecution();

	void SetRenderTargets();
	void ClearRenderTarget();
	void ClearDepthBuffer();


	void OffscreenUpDate();
	void OffscreenDebugGUI();

	/// <summary>
	/// バリアの変更
	/// </summary>
	/// <param name="resource">リソース</param>
	/// <param name="before">変更前の状態</param>
	/// <param name="after">変更後の状態</param>
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	void InsertUAVBarrier(ID3D12Resource* resource);
	void PreOutline();
	void PostOutline();

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffscreenTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVResource(ID3D12Device* device, size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateReadbackResource(ID3D12Device* device, size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(size_t sizeInBytes, const void* initData);


	ID3D12Device* GetDevice() const { return device_.Get(); }
	size_t GetBackBufferCount() const { return swapChainDesc_.BufferCount; }
	ID3D12GraphicsCommandList* GetCommandList() const { return command_->GetList(); }
	DXCommand* GetDXCommand() const { return command_.get(); }
	DXCompil* GetDXCompil() const { return compiler_.get(); }
	PipelineManager* GetPipelineManager()const { return pipeManager_; }
	OffscreenManager* GetOffscreenManager()const { return offscreen_.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() { return rtvHandles_[2]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetDepthTexGPUHandle() { return depthTexSrvHandle_.second; }

	/*void Tick();*/

private:

	void CreateDevice();

	void CreateCommand();

	void CreateSwapChain();

	void CreateRenderTargets();

	void CreateDepthBuffer();

	void CreateCompiler();

	void InitializeFPSKeeper();

	void SettingRootSignature();
	void SettingGraphicPipeline();

	/// <summary>
	/// swapchainのバリア
	/// </summary>
	/// <param name="before"></param>
	/// <param name="after"></param>
	void CreateBarrier(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);


private:

	D3DResourceLeakChecker leakCheck_;

	MyWin* myWin_;


#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;
#endif // _DEBUG


	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;


	std::unique_ptr<DXCommand> command_ = nullptr;


	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[3];

	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc_{};
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc_{};
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> depthTexSrvHandle_;


	std::unique_ptr<DXCompil> compiler_ = nullptr;

	FPSKeeper* fpsKeeper_ = nullptr;

	PipelineManager* pipeManager_;

	std::unique_ptr<OffscreenManager> offscreen_;

};
