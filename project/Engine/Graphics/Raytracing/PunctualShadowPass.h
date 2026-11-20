#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"
#include "Engine/Graphics/Raytracing/Denoiser.h"
#include "Engine/Graphics/Raytracing/WorkTexture.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class GBufferPass;
	class LightManager;

	/// <summary>
	/// 点光源とスポットの柔らかい影を画面空間で3本まで計算し、まとめてデノイズするパス
	/// </summary>
	/// <remarks>sourceRadius を持つ影ありの光源をカメラに近い順に選ぶ。残りは硬い影</remarks>
	class PunctualShadowPass {
	public:
		PunctualShadowPass() = default;
		~PunctualShadowPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>柔らかくする光源を選び、このフレームのライトの定数へ枠を入れる</summary>
		/// <remarks>G-Buffer のプリパスを走らせるかを決める前に呼ぶ</remarks>
		void Prepare();
		/// <summary>Prepare で光源を1本でも選んだか</summary>
		bool HasLights() const { return slotCount_ > 0; }

		/// <summary>選んだ光源の遮蔽率を計算してデノイズする</summary>
		/// <remarks>G-Buffer と TLAS が済んでいること。走らせないフレームは枠を空ける</remarks>
		void Render(const GBufferPass& gbuffer);

		void DebugGUI();

		/// <summary>前方描画がバインドする遮蔽率テクスチャ (rgb が枠 0〜2)</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetShadowSrvHandle() const { return denoiser_.GetResultSrvHandle(); }

	private:

		/// <summary>HLSL 側 (Raytracing/PunctualShadow.CS.hlsl) の PunctualShadowParam と並びが一致していること</summary>
		struct PunctualShadowParam {
			uint32_t screenWidth = 0;
			uint32_t screenHeight = 0;
			// サンプルの向きをフレームごとにずらす量。[0,1)
			float temporalJitter = 0.0f;
			uint32_t pad = 0;
		};

		static constexpr uint32_t kTimer_Trace = 0;

		/// <summary>枠を全部空け、蓄積を捨てる</summary>
		void Disable();

		/// <summary>柔らかくする光源を選ぶ。前のフレームと同じ光源は同じ枠に置く</summary>
		/// <returns>AllLightsData::softShadowLights に入れる値</returns>
		uint32_t SelectLights() const;

		void UpdateParam(uint32_t frameIndex);
		void DispatchTrace(uint32_t frameIndex, const GBufferPass& gbuffer, uint32_t tlasSrvIndex);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		WorkTexture raw_;
		Denoiser denoiser_;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		PunctualShadowParam* paramMapped_[DXC::kFrameCount_] = {};

		uint64_t frameCounter_ = 0;
		// このフレームの枠 (AllLightsData::softShadowLights と同じ形)
		uint32_t slots_ = 0xFFFFFFFFu;
		uint32_t slotCount_ = 0;

		DXC::GPUTimer gpuTimer_;
	};

}
