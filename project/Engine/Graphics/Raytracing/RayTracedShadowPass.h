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
	/// G-Buffer を元に画面空間で平行光源のソフトシャドウを計算し、デノイズまで掛けるパス
	/// </summary>
	/// <remarks>
	/// 遮蔽率は光源ごとに別の値なのでバッファ1枚では1本ぶんしか持てない。
	/// このパスが受け持つのは0番の平行光源だけで、残りは前方描画のPSがハードシャドウを飛ばす
	/// </remarks>
	class RayTracedShadowPass {
	public:
		RayTracedShadowPass() = default;
		~RayTracedShadowPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームの遮蔽率を計算してデノイズする</summary>
		/// <remarks>
		/// G-Bufferの書き出しが済んでいること。TLASも組み終わっていること。
		/// 影が Soft 以外なら何もせず蓄積だけ捨てるので、呼び出し側で切り分けなくてよい
		/// </remarks>
		void Render(const GBufferPass& gbuffer);

		void DebugGUI();

		//========================================================================*/
		//* Getter

		/// <summary>前方描画がバインドする遮蔽率テクスチャ</summary>
		/// <remarks>デノイザの設定で実体が変わるので、Render を通した後の値を使うこと</remarks>
		D3D12_GPU_DESCRIPTOR_HANDLE GetShadowSrvHandle() const { return denoiser_.GetResultSrvHandle(); }

	private:

		/// <summary>CS へ渡す、ライト設定に無いパラメータ</summary>
		/// <remarks>HLSL 側 (Raytracing/RayTracedShadow.CS.hlsl) の ShadowPassParam と並びが一致していること</remarks>
		struct ShadowPassParam {
			uint32_t screenWidth = 0;
			uint32_t screenHeight = 0;
			// サンプルの向きをフレームごとにずらす量。[0,1)
			float temporalJitter = 0.0f;
			uint32_t pad = 0;
		};

		static constexpr uint32_t kTimer_Trace = 0; // レイを飛ばして遮蔽率を求めるところ

		/// <summary>1フレーム分の定数を書き込む</summary>
		void UpdateParam(uint32_t frameIndex);

		/// <summary>生の遮蔽率を求める</summary>
		void DispatchTrace(uint32_t frameIndex, const GBufferPass& gbuffer, uint32_t tlasSrvIndex);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		// トレースの出力。デノイザが同じフレームのうちに読むだけなので1枚でよい
		WorkTexture raw_;
		Denoiser denoiser_;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		ShadowPassParam* paramMapped_[DXC::kFrameCount_] = {};

		// 黄金比でずらすと少ない本数でも偏りにくい
		uint64_t frameCounter_ = 0;

		DXC::GPUTimer gpuTimer_;
	};

}
