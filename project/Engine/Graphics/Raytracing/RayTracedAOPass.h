#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/Matrix4x4.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class GBufferPass;
	class LightManager;

	/// <summary>
	/// G-Buffer を元に画面空間でレイトレAOを計算し、デノイズまで掛けるパス
	/// </summary>
	/// <remarks>設定値は AllLights と共用するので専用の定数バッファはデノイザぶんしか持たない</remarks>
	class RayTracedAOPass {
	public:
		RayTracedAOPass() = default;
		~RayTracedAOPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームのAOを計算してデノイズする</summary>
		/// <remarks>G-Bufferの書き出しが済んでいること。TLASも組み終わっていること</remarks>
		void Render(const GBufferPass& gbuffer);

		void DebugGUI();

		//========================================================================*/
		//* Getter

		/// <summary>前方描画がバインドするAOテクスチャ</summary>
		/// <remarks>デノイザの設定で実体が変わるので、Render を通した後の値を使うこと</remarks>
		D3D12_GPU_DESCRIPTOR_HANDLE GetAOSrvHandle() const;

	private:

		/// <summary>UAVとSRVを1枚ずつ張った作業用テクスチャ</summary>
		struct WorkTexture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
		};

		/// <summary>CS へ渡す、ライト設定に無いパラメータ</summary>
		/// <remarks>HLSL 側 (Raytracing/RayTracedAO.CS.hlsl) の AOPassParam と並びが一致していること</remarks>
		struct AOPassParam {
			uint32_t screenWidth = 0;
			uint32_t screenHeight = 0;
			// サンプルの向きをフレームごとにずらす量。[0,1)
			float temporalJitter = 0.0f;
			uint32_t pad = 0;
		};

		/// <summary>HLSL 側 (AODenoiseTemporal.CS.hlsl) の DenoiseTemporalParam と並びが一致していること</summary>
		struct DenoiseTemporalParam {
			Math::Matrix4x4 prevViewProj;
			Math::Matrix4x4 prevInvViewProj;
			uint32_t screenWidth = 0;
			uint32_t screenHeight = 0;
			uint32_t hasHistory = 0;
			float maxHistoryLength = 0.0f;
			float normalThreshold = 0.0f;
			float positionThreshold = 0.0f;
			float pad[2]{};
		};

		/// <summary>HLSL 側 (AODenoiseSpatial.CS.hlsl) の DenoiseSpatialParam と並びが一致していること</summary>
		struct DenoiseSpatialParam {
			uint32_t screenWidth = 0;
			uint32_t screenHeight = 0;
			uint32_t stepWidth = 1;
			float normalPower = 0.0f;
			float planeScale = 0.0f;
			// タップを並べる向き。(1,0)で横、(0,1)で縦
			int32_t tapDirection[2]{};
			float pad = 0.0f;
		};

		/// <summary>à-trous を掛けられる最大回数</summary>
		/// <remarks>1回ごとに間隔が倍になるので、5回で 32画素ぶんの広がりになる</remarks>
		static constexpr uint32_t kMaxSpatialIterations_ = 5;

		/// <summary>1回を横と縦に割っているので、ディスパッチも定数も1回につき2つ要る</summary>
		static constexpr uint32_t kSpatialPassesPerIteration_ = 2;

		// ルートCBVのアドレスは256バイト境界に載っている必要がある
		static constexpr uint64_t kSpatialParamStride_ = 256;

		enum TimerId : uint32_t {
			kTimer_Trace = 0,   // レイを飛ばしてAOを求めるところ
			kTimer_Temporal,    // 時間方向の蓄積
			kTimer_Spatial,     // 空間フィルタ(全回数の合計)
			kTimer_Count
		};

		/// <summary>作業用テクスチャを1枚作り、SRVとUAVを張る</summary>
		WorkTexture CreateWorkTexture(DXGI_FORMAT format, bool clearToOpen);

		/// <summary>1フレーム分の定数を書き込む</summary>
		void UpdateParam(uint32_t frameIndex, const GBufferPass& gbuffer);

		/// <summary>生AOを求める</summary>
		void DispatchTrace(uint32_t frameIndex, const GBufferPass& gbuffer, uint32_t tlasSrvIndex);
		/// <summary>前フレームまでの蓄積と混ぜる</summary>
		void DispatchTemporal(uint32_t frameIndex, const GBufferPass& gbuffer);
		/// <summary>à-trous を1回ぶん、横か縦の片方向だけ掛ける</summary>
		/// <param name="paramSlot">定数バッファの何番目を使うか。方向ごとに別の中身が要る</param>
		void DispatchSpatial(uint32_t frameIndex, const GBufferPass& gbuffer,
			uint32_t iteration, uint32_t paramSlot, bool vertical,
			uint32_t srcSrvIndex, const WorkTexture& dst);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		// トレースの出力。前方描画が同じフレームのうちに読むだけなので1枚でよい
		WorkTexture raw_;
		// 次フレームへ渡す蓄積。R16G16_FLOAT で x=AO, y=蓄積フレーム数
		WorkTexture history_[DXC::kFrameCount_];
		// à-trous のピンポン。時間方向の出力もここの[0]へ書く
		WorkTexture filter_[2];

		// 前方描画が読むもの。デノイザの設定によって raw_ にも filter_ にもなる
		uint32_t finalSrvIndex_ = 0;

		// 生成直後に「遮蔽なし」で塗るためだけのRTV。以後は使わない
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> clearRtvHeap_;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		AOPassParam* paramMapped_[DXC::kFrameCount_] = {};
		Microsoft::WRL::ComPtr<ID3D12Resource> temporalResource_[DXC::kFrameCount_];
		DenoiseTemporalParam* temporalMapped_[DXC::kFrameCount_] = {};
		// 回数ぶんを1本にまとめ、256バイトずつずらして参照する
		Microsoft::WRL::ComPtr<ID3D12Resource> spatialResource_[DXC::kFrameCount_];
		uint8_t* spatialMapped_[DXC::kFrameCount_] = {};

		// 黄金比でずらすと少ない本数でも偏りにくい
		uint64_t frameCounter_ = 0;

		// 再投影に使う。カメラは毎フレーム動くので前フレームの行列を持っておく
		Math::Matrix4x4 prevViewProj_;
		Math::Matrix4x4 prevInvViewProj_;
		// 前フレームが蓄積を書いたか。AOを入れ直した直後は履歴が古いので見てはいけない
		bool hasHistory_ = false;

		bool enableTemporal_ = true;
		int32_t spatialIterations_ = 3;
		float maxHistoryLength_ = 32.0f;
		float normalThreshold_ = 0.9f;
		float positionThreshold_ = 0.02f;
		float normalPower_ = 64.0f;
		float planeScale_ = 0.02f;

		DXC::GPUTimer gpuTimer_;
	};

}
