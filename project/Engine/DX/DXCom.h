
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


/// <summary>
/// リークチェック
/// </summary>
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


/// <summary>
/// DXComクラス
/// </summary>
class DXCom {
public:
	DXCom() = default;
	~DXCom() = default;
public:

	void Initialize(Core::MyWin* myWin);
	void Finalize();

	/// <summary>
	/// Finalize前に先にGPUが処理終わるように
	/// </summary>
	void Flush();

	/// <summary>
	/// PostEffect用Texture等の初期化
	/// </summary>
	void SettingTexture();

	/// <summary>
	/// 描画開始時
	/// </summary>
	void PreDraw();

	/// <summary>
	/// 基本パイプラインにする関数
	/// </summary>
	void Command();

	/// <summary>
	/// ポストエフェクト
	/// </summary>
	void PostEffect();

	/// <summary>
	/// 描画終了
	/// </summary>
	void PostDraw();

	/// <summary>
	/// フレームの最初に入れる処理
	/// </summary>
	void BeginFrame();

	/// <summary>
	/// シーンを変える前に1フレ前を待つ
	/// </summary>
	void PerFrameWait();

	/// <summary>
	/// スプライト描画前に呼び出す関数
	/// </summary>
	void PreSpriteDraw();

	/// <summary>
	/// 実行
	/// </summary>
	void CommandExecution();

	//========================================================================*/
	//* Frameの描画前に呼び出す関数群
	void SetRenderTargets(); // レンダーターゲットのセット
	void ClearRenderTarget(); // レンダーターゲットのクリア
	void ClearDepthBuffer(); // 深度バッファのクリア

	//========================================================================*/
	//* オフスクリーンの処理を行う関数群
	void OffscreenUpDate(); // オフスクリーンの処理Update
	void OffscreenDebugGUI(); // オフスクリーン用DebugGUI

	/// <summary>
	/// バリアの変更
	/// </summary>
	/// <param name="resource">リソース</param>
	/// <param name="before">変更前の状態</param>
	/// <param name="after">変更後の状態</param>
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	/// <summary>
	/// UAV（Unordered Access View）バリアを挿入する
	/// </summary>
	/// <param name="resource">リソース</param>
	void InsertUAVBarrier(ID3D12Resource* resource);

	/// <summary>
	/// UAV（Unordered Access View）バリアをComputeへ挿入する
	/// </summary>
	/// <param name="resource">リソース</param>
	void InsertUAVBarrierForCompute(ID3D12Resource* resource);

	/// <summary>
	/// OutLine実行前に呼び出す関数
	/// </summary>
	void PreOutline();

	/// <summary>
	/// OutLine実行後に呼び出す関数
	/// </summary>
	void PostOutline();


	/// <summary>
	/// 指定されたサイズのバッファリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

	/// <summary>
	/// 指定されたタイプ・数でディスクリプタヒープを生成する。
	/// </summary>
	/// <param name="heapType">ヒープタイプ</param>
	/// <param name="numDescriptors">数</param>
	/// <param name="shaderVisible">shaderVisible</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 深度ステンシル用のテクスチャリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="width">横</param>
	/// <param name="height">縦</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);

	/// <summary>
	/// オフスクリーン描画用のテクスチャリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="width">横</param>
	/// <param name="height">縦</param>
	/// <param name="color">色</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffscreenTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color);

	/// <summary>
	/// UAV（Unordered Access View）として使用可能なバッファリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// GPUからCPUへのデータ読み戻しに使用するリードバックリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateReadbackResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// CPUからGPUへデータ転送するためのアップロードバッファを生成する。
	/// </summary>
	/// <param name="sizeInBytes">サイズ</param>
	/// <param name="initData">データ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(size_t sizeInBytes, const void* initData);

	//========================================================================*/
	//* Getter
	ID3D12Device* GetDevice() const { return device_.Get(); }
	size_t GetBackBufferCount() const { return swapChainDesc_.BufferCount; }
	uint32_t GetNowFrameCount() const { return command_->GetNowFrameIndex(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return command_->GetList(); }
	ID3D12GraphicsCommandList* GetComputeCommandList() const { return command_->GetComputeList(); }
	ID3D12GraphicsCommandList* GetImmediateList() const { return command_->GetImmediateList(); }
	DXC::DXCommand* GetDXCommand() const { return command_.get(); }
	DXC::DXCompil* GetDXCompil() const { return compiler_.get(); }
	Graphics::PipelineManager* GetPipelineManager()const { return pipeManager_; }
	Graphics::OffscreenManager* GetOffscreenManager()const { return offscreen_.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(uint32_t index) { return rtvHandles_[2 + index]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetDepthTexGPUHandle() { return depthTexSrvHandle_[GetNowFrameCount()].second; }

	/*void Tick();*/

private:
	/// <summary>
	/// デバイス作成
	/// </summary>
	void CreateDevice();

	/// <summary>
	/// コマンド作成
	/// </summary>
	void CreateCommand();

	/// <summary>
	/// スワップチェイン作成
	/// </summary>
	void CreateSwapChain();

	/// <summary>
	/// レンダーターゲット作成
	/// </summary>
	void CreateRenderTargets();

	/// <summary>
	/// 深度バッファ作成
	/// </summary>
	void CreateDepthBuffer();

	/// <summary>
	/// コンパイラー作成
	/// </summary>
	void CreateCompiler();

	/// <summary>
	/// FPSKeeperの作成
	/// </summary>
	void InitializeFPSKeeper();

	/// <summary>
	/// ルートシグネーチャーの設定
	/// </summary>
	void SettingRootSignature();

	/// <summary>
	/// swapChainのバリア
	/// </summary>
	/// <param name="before">前</param>
	/// <param name="after">後</param>
	void CreateBarrier(D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);


private:

	D3DResourceLeakChecker leakCheck_;

	Core::MyWin* myWin_;


#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;
#endif // _DEBUG


	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;


	std::unique_ptr<DXC::DXCommand> command_ = nullptr;


	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[4];
	UINT numRTVHandle_ = 4;

	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_[DXC::kFrameCount_];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc_{};
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc_{};
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> depthTexSrvHandle_[DXC::kFrameCount_];


	std::unique_ptr<DXC::DXCompil> compiler_ = nullptr;

	Core::FPSKeeper* fpsKeeper_ = nullptr;

	Graphics::PipelineManager* pipeManager_;

	std::unique_ptr<Graphics::OffscreenManager> offscreen_;

};
