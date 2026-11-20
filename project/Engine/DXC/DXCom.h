#pragma once
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <cassert>
#include <wrl.h>

#include "Engine/DXC/Device/GraphicsDevice.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Device/SwapChainManager.h"


namespace Core {
	class MyWin;
	class FPSKeeper;
}
namespace Graphics {
	class PipelineManager;
	class OffscreenManager;
}

namespace DXC {

	class DXCompile;
	class GPUTimer;
	class FrameProfiler;

	/// <summary>
	/// 1フレームの描画を大きく分けた区間。GPU 時間の一覧に出る
	/// </summary>
	enum class GpuSection : uint32_t {
		Frame,
		Skinning,
		Prepare,
		Sky,
		Objects,
		Particles,
		Sprites,
		PostEffect,
		DebugGui,
		Count
	};

	/// <summary>
	/// DXComクラス
	/// </summary>
	class DXCom {
	public:
		DXCom();
		// unique_ptrの中身が不完全型なのでデストラクタは.cppに置く
		~DXCom();
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

		/// <summary>ウィンドウの大きさが変わっていたら、バックバッファを作り直す</summary>
		/// <remarks>作り直す前に GPU を待つので、変わったフレームだけ一瞬止まる</remarks>
		void ResizeSwapChainIfNeeded();

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
		void SetRenderTargetsForGPUParticle(); // レンダーターゲットのセット
		void ClearRenderTarget(); // レンダーターゲットのクリア
		void ClearRenderTargetForGPUParticle(); // レンダーターゲットのクリア
		void ClearDepthBuffer(); // 深度バッファのクリア

		//========================================================================*/
		//* オフスクリーンの処理を行う関数群
		void OffscreenUpdate(); // オフスクリーンの処理Update
		void OffscreenDebugGUI(); // オフスクリーン用DebugGUI
		D3D12_GPU_DESCRIPTOR_HANDLE GetGameViewTexture() const; // デバッグGUIのパネルに貼るゲーム画面

		//========================================================================*/
		//* GPUParticleの描画処理を行う際の関数群
		void PreGPUParticleDraw();
		void PostGPUParticleDraw();

		//========================================================================*/
		//* 深度バッファをCSから読む/DSVへ戻す状態遷移 (splatのシーン遮蔽深度テスト等で使用)
		void TransitionDepthToRead();   // DEPTH_WRITE -> NON_PIXEL_SHADER_RESOURCE
		void TransitionDepthToWrite();  // NON_PIXEL_SHADER_RESOURCE -> DEPTH_WRITE

		/// <summary>このフレームの本描画の深度バッファ</summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferHandle() const { return swapChainManager_->GetDSVHandle(GetNowFrameCount()); }
		/// <summary>本描画の深度バッファの中身を同じ形式の dest へ写す</summary>
		/// <remarks>深度バッファが DEPTH_WRITE の間に呼ぶ。dest は destState へ戻す</remarks>
		void CopyFromDepthBuffer(ID3D12Resource* dest, D3D12_RESOURCE_STATES destState);

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

		//========================================================================*/
		//* Getter
		ID3D12Device* GetDevice() const { return graphicsDevice_->GetDevice(); }
		ID3D12Device5* GetDevice5() const { return graphicsDevice_->GetDevice5(); }
		bool IsRayQuerySupported() const { return graphicsDevice_->IsRayQuerySupported(); }
		size_t GetBackBufferCount() const { return swapChainManager_->GetBackBufferCount(); }
		uint32_t GetNowFrameCount() const { return command_->GetNowFrameIndex(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return command_->GetList(); }
		ID3D12GraphicsCommandList* GetComputeCommandList() const { return command_->GetComputeList(); }
		ID3D12GraphicsCommandList* GetImmediateList() const { return command_->GetImmediateList(); }
		DXC::DXCommand* GetDXCommand() const { return command_.get(); }
		DXC::DXCompile* GetDXCompile() const { return compiler_.get(); }
		Graphics::PipelineManager* GetPipelineManager()const { return pipeManager_; }
		Graphics::OffscreenManager* GetOffscreenManager()const { return offscreen_.get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetOffscreenRTV(uint32_t frameIndex) { return swapChainManager_->GetOffscreenRTV(frameIndex); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetGPUParticleRTV(uint32_t frameIndex) { return swapChainManager_->GetGPUParticleRTV(frameIndex); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetDepthTexGPUHandle() { return swapChainManager_->GetDepthTexGPUHandle(GetNowFrameCount()); }

		/// <summary>1つ前のフレームで GPU が描画に使った時間(ms)。垂直同期や上限の待ちは含まない</summary>
		double GetGpuFrameMS() const;

		/// <summary>描画の区間の GPU 時間を計る。Begin と End で挟む</summary>
		void BeginGpuSection(GpuSection section);
		void EndGpuSection(GpuSection section);

		/// <summary>決めたフレーム数の平均を取ってファイルへ書く計測モード</summary>
		DXC::FrameProfiler* GetFrameProfiler() const { return frameProfiler_.get(); }

	private:


	private:

		Core::MyWin* myWin_;

		std::unique_ptr<DXC::GraphicsDevice> graphicsDevice_;
		std::unique_ptr<DXC::DXCommand> command_ = nullptr;
		std::unique_ptr<DXC::SwapChainManager> swapChainManager_ = nullptr;
		std::unique_ptr<DXC::DXCompile> compiler_ = nullptr;
		std::unique_ptr<DXC::GPUTimer> gpuFrameTimer_;
		std::unique_ptr<DXC::FrameProfiler> frameProfiler_;

		Core::FPSKeeper* fpsKeeper_ = nullptr;
		Graphics::PipelineManager* pipeManager_;
		std::unique_ptr<Graphics::OffscreenManager> offscreen_;
	};

}
