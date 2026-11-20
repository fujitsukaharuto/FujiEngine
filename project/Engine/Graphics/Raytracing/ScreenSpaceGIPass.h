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
	class ScreenReflectionPass;

	/// <summary>
	/// 画面の照り返し (SSGI)。画面に写っている物の色を、近くの面へ間接光として足す
	/// </summary>
	/// <remarks>乱数を使わないので止まっていればちらつかない。画面の外の物は照り返さない</remarks>
	class ScreenSpaceGIPass {
	public:
		struct Settings {
			float intensity = 1.0f;		// 照り返しに掛ける倍率
			float maxDistance = 3.0f;	// レイの届く距離 (m)
		};

		ScreenSpaceGIPass() = default;
		~ScreenSpaceGIPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームの照り返しを求める</summary>
		/// <remarks>G-Buffer の後、前方描画の前に呼ぶ。色は sceneColor が写した前のフレームの画面から取る</remarks>
		void Render(const GBufferPass& gbuffer, const ScreenReflectionPass& sceneColor);

		void SetSettings(const Settings& settings) { settings_ = settings; }

		void DebugGUI();

		/// <summary>前方描画が引く結果。rgb=照り返し, a=遮られた割合</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetResultSrvHandle() const;

	private:

		/// <summary>HLSL 側 (Common/ScreenSpaceGICommon.hlsli) の SSGIPassParam と並びが一致していること</summary>
		struct SSGIPassParam {
			Math::Matrix4x4 sceneColorViewProj;
			uint32_t screenSize[2] = {};
			float maxDistance = 0.0f;
			float intensity = 0.0f;
			int32_t blurDirection[2] = {};
			uint32_t useSceneColor = 0;
			uint32_t blurWide = 0;
			float depthToViewZ[2] = {};
			float pad[2] = {};
		};

		// 定数の並び: レイ → 模様を消す横・縦 → 広いぼかしの横・縦。256 バイト境界
		static constexpr uint64_t kParamStride_ = 256;
		static constexpr uint32_t kParamSlots_ = 5;
		static_assert(sizeof(SSGIPassParam) == 112, "SSGIPassParam の大きさが ScreenSpaceGICommon.hlsli と一致していない");

		static constexpr uint32_t kThreadGroupSize_ = 8;

		enum TimerID : uint32_t {
			kTimer_Trace = 0,
			kTimer_Blur,
			kTimer_Count,
		};

		void UpdateParam(uint32_t frameIndex, const ScreenReflectionPass& sceneColor);
		/// <summary>1方向ぶんぼかす</summary>
		void DispatchBlur(uint32_t frameIndex, uint32_t paramSlot, const GBufferPass& gbuffer,
			const WorkTexture& input, WorkTexture& output);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		Settings settings_;

		// レイの結果、横にぼかした途中、縦までぼかした結果
		WorkTexture trace_;
		WorkTexture blurTemp_;
		WorkTexture result_;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		uint8_t* paramMapped_[DXC::kFrameCount_] = {};

		DXC::GPUTimer gpuTimer_;
	};

}
