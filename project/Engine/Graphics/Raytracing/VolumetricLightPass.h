#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"
#include "Engine/Graphics/Raytracing/WorkTexture.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class ClusteredLightPass;
	class GBufferPass;
	class LightManager;
	class RaytracingScene;

	/// <summary>
	/// 光の筋 (ボリュメトリックライト)。空気の中を進みながら太陽へ影のレイを飛ばす
	/// </summary>
	/// <remarks>縦横 1/4 の解像度で求めてぼかし、OffscreenManager が画面へ足す</remarks>
	class VolumetricLightPass {
	public:
		/// <summary>配置データの環境が入れる設定</summary>
		struct Settings {
			float density = 0.0f;		// 1m あたりの散乱。0で切る
			float anisotropy = 0.6f;	// 散乱の向きの偏り
			float maxDistance = 50.0f;	// カメラからこの距離までを見る
			float intensity = 1.0f;		// 散った光の倍率。霞を濃くせず筋だけ明るくする
			float sunScale = 1.0f;		// 太陽の光の筋の倍率。0でレイを飛ばさない
			float localLightScale = 0.0f;	// 影なしのライトが霧でにじむ倍率
		};

		VolumetricLightPass() = default;
		~VolumetricLightPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		void SetSettings(const Settings& settings) { settings_ = settings; }
		bool IsEnabled() const { return settings_.density > 0.0f; }

		/// <summary>このフレームの光の筋を求める</summary>
		/// <remarks>G-Buffer・TLAS・影なしのライトの振り分けが済んでいること</remarks>
		void Render(const GBufferPass& gbuffer, const RaytracingScene& scene, const ClusteredLightPass& clustered);

		/// <summary>このフレームに求めたか。求めていなければ結果は古い</summary>
		bool IsRendered() const { return isRendered_; }

		/// <summary>画面へ足す結果。rgb=散った光, a=ビュー空間の奥行き</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetResultSrvHandle() const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

		void DebugGUI();

	private:

		/// <summary>HLSL 側 (Common/VolumetricLightCommon.hlsli) の VolumetricParam と並びが一致していること</summary>
		struct VolumetricParam {
			uint32_t lowSize[2] = {};
			uint32_t screenSize[2] = {};
			float density = 0.0f;
			float anisotropy = 0.0f;
			float maxDistance = 0.0f;
			float intensity = 0.0f;
			float depthToView[2] = {};
			float sunScale = 0.0f;
			float localLightScale = 0.0f;
		};

		// 画面を縦横これで割った解像度で求める
		static constexpr uint32_t kDownscale_ = 4;
		static constexpr uint32_t kThreadGroupSize_ = 8;

		enum TimerID : uint32_t {
			kTimer_Trace = 0,
			kTimer_Blur,
			kTimer_Count,
		};

		void UpdateParam(uint32_t frameIndex);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		Settings settings_;
		bool isRendered_ = false;

		// 進みながら足した光と、それをぼかした結果
		WorkTexture trace_;
		WorkTexture result_;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		VolumetricParam* paramMapped_[DXC::kFrameCount_] = {};

		DXC::GPUTimer gpuTimer_;
	};

}
