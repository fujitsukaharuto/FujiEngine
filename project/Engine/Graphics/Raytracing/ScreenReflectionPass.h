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
	/// 画面の映り込み。画素ごとに鏡の向きへレイを1本飛ばし、粗さに応じてぼかす
	/// </summary>
	/// <remarks>乱数を使わないので止まっていればちらつかない。粗い面は画面のプローブで代える</remarks>
	class ScreenReflectionPass {
	public:
		ScreenReflectionPass() = default;
		~ScreenReflectionPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームの映り込みを求める</summary>
		/// <remarks>G-Buffer・TLAS・格子のプローブの更新が済んでいること</remarks>
		void Render(const GBufferPass& gbuffer, const RaytracingScene& scene);

		/// <summary>不透明の物を描き終えた画面を写す。次のフレームのレイが引く</summary>
		/// <remarks>半透明・加算の物を描く前に呼ぶ</remarks>
		void CaptureSceneColor();

		void DebugGUI();

		/// <summary>濡れた路面のように、映り込みを縦に伸ばす強さ。0で伸ばさない</summary>
		void SetStretch(float stretch) { stretch_ = stretch; }

		/// <summary>前方描画が引く結果。rgb=当たった物の明るさ, a=0.5+空が見えた割合/2 (0なら使えない)</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetResultSrvHandle() const;

	private:

		/// <summary>HLSL 側 (Common/ScreenReflectionCommon.hlsli) の ReflectionPassParam と並びが一致していること</summary>
		struct ReflectionPassParam {
			uint32_t screenSize[2] = {};
			uint32_t reflectAll = 0;
			float maxRoughness = 0.0f;
			float maxRayDistance = 0.0f;
			float pixelsPerUnit = 0.0f;
			float pad[2] = {};
			int32_t blurDirection[2] = {};
			uint32_t inputIsTrace = 0;
			float stretch = 0.0f;
			Math::Matrix4x4 sceneColorViewProj;	// 写した画面の ViewProjection (前のフレーム)
			uint32_t useSceneColor = 0;
			float sceneColorPad[3] = {};
		};

		// 定数の並び: レイ → 横のぼかし → 縦のぼかし。256 バイト境界
		static constexpr uint64_t kParamStride_ = 256;
		static constexpr uint32_t kParamSlots_ = 3;
		static_assert(sizeof(ReflectionPassParam) <= kParamStride_);

		// これより粗い面からは飛ばさない
		static constexpr float kMaxRoughness_ = 0.4f;
		static constexpr uint32_t kThreadGroupSize_ = 8;

		enum TimerID : uint32_t {
			kTimer_Trace = 0,
			kTimer_Blur,
			kTimer_Count,
		};

		void UpdateParam(uint32_t frameIndex);
		bool IsSceneColorEnabled() const;
		/// <summary>1方向ぶんぼかす</summary>
		void DispatchBlur(uint32_t frameIndex, uint32_t paramSlot, const GBufferPass& gbuffer,
			const WorkTexture& input, WorkTexture& output);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		// レイの結果、横にぼかした途中、縦までぼかした結果
		WorkTexture trace_;
		WorkTexture blurTemp_;
		WorkTexture result_;
		// 前のフレームの、不透明の物を描き終えた画面と、その ViewProjection
		WorkTexture sceneColor_;
		Math::Matrix4x4 sceneColorViewProj_;
		bool hasSceneColor_ = false;
		float stretch_ = 0.0f;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		uint8_t* paramMapped_[DXC::kFrameCount_] = {};

		DXC::GPUTimer gpuTimer_;
	};

}
