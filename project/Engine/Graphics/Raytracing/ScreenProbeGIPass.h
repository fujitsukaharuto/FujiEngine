#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"
#include "Engine/Graphics/Raytracing/WorkTexture.h"
#include "Engine/Math/Matrix/Matrix4x4.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class GBufferPass;
	class LightManager;
	class RaytracingScene;

	/// <summary>
	/// 画面のプローブによる間接光 (Lumen の Screen Probe Gather と同じ考え方)
	/// </summary>
	/// <remarks>16x16 画素ごとにプローブを置いてレイを飛ばす。当たった先は格子のプローブで光らせる</remarks>
	class ScreenProbeGIPass {
	public:
		ScreenProbeGIPass() = default;
		~ScreenProbeGIPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームの間接光を求める</summary>
		/// <remarks>G-Buffer・TLAS・格子のプローブの更新が済んでいること</remarks>
		void Render(const GBufferPass& gbuffer, const RaytracingScene& scene);

		void DebugGUI();

		/// <summary>前方描画が引く結果。rgb=間接光, a=使えるか</summary>
		/// <remarks>Render のたびに実体が入れ替わる</remarks>
		D3D12_GPU_DESCRIPTOR_HANDLE GetResultSrvHandle() const;

	private:

		/// <summary>HLSL 側 (Common/ScreenProbeCommon.hlsli) の ScreenProbeParam と並びが一致していること</summary>
		struct ScreenProbeParam {
			Math::Matrix4x4 prevViewProj;
			Math::Matrix4x4 prevInvViewProj;
			uint32_t screenSize[2] = {};
			uint32_t probeCount[2] = {};
			uint32_t tileSize = 0;
			uint32_t frameIndex = 0;
			uint32_t hasHistory = 0;
			float maxHistory = 0.0f;
			float maxRayDistance = 0.0f;
			float pixelsPerUnit = 0.0f;
			float pad[2] = {};
		};

		// タイルの1辺の画素数
		static constexpr uint32_t kTileSize_ = 16;
		// プローブ1つあたりのレイ。HLSL 側 (kScreenProbeRays) と一致していること
		static constexpr uint32_t kRaysPerProbe_ = 64;
		// 時間方向に混ぜる上限のフレーム数
		static constexpr float kMaxHistory_ = 32.0f;
		// 隣と混ぜる回数 (Lumen の SpatialFilterNumPasses)
		static constexpr uint32_t kFilterPasses_ = 3;
		static constexpr float kMaxRayDistance_ = 200.0f;
		static constexpr uint32_t kThreadGroupSize_ = 8;

		enum TimerID : uint32_t {
			kTimer_Trace = 0,
			kTimer_Resolve, // 混ぜる・引く・時間方向
			kTimer_Count,
		};

		void UpdateParam(uint32_t frameIndex);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		uint32_t probeCountX_ = 0;
		uint32_t probeCountY_ = 0;

		// プローブごと。係数は横に R,G,B の3枚
		WorkTexture probeSH_;
		WorkTexture probeSHFiltered_;
		WorkTexture probePosition_;
		WorkTexture probeNormal_;
		// 画素ごと。引いた値と、時間方向に混ぜた結果 (2枚を入れ替える)
		WorkTexture raw_;
		WorkTexture history_[2];
		WorkTexture historyCount_[2];
		uint32_t current_ = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		ScreenProbeParam* paramMapped_[DXC::kFrameCount_] = {};

		// 再投影に使う、前のフレームの行列
		Math::Matrix4x4 prevViewProj_;
		Math::Matrix4x4 prevInvViewProj_;
		// 前のフレームが結果を書いたか
		bool hasHistory_ = false;
		uint32_t frameCounter_ = 0;

		DXC::GPUTimer gpuTimer_;
	};

}
