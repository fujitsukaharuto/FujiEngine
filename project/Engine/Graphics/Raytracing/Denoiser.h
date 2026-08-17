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

	/// <summary>
	/// 画面空間の1チャンネル信号から、レイの本数が足りないことによる粒を落とすクラス
	/// </summary>
	/// <remarks>
	/// 前フレームへ再投影して蓄積する時間方向と、à-trous のエッジ保持ブラーを掛ける空間方向の2段。
	/// AOも影も入出力が1チャンネルで中身が同じなので、信号を持つパスが1つずつこれを持つ形にしてある
	/// </remarks>
	class Denoiser {
	public:
		Denoiser() = default;
		~Denoiser() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>生の信号をならす。結果は GetResultSrvHandle() から引く</summary>
		/// <param name="gbuffer">エッジの判定に使う。書き出しが済んでいること</param>
		/// <param name="raw">ならす対象。このフレームの全画素が書かれていること</param>
		void Render(const GBufferPass& gbuffer, const WorkTexture& raw);

		/// <summary>蓄積を捨てる。パスを止めていた間の履歴は当てにならない</summary>
		void ResetHistory() { hasHistory_ = false; }

		/// <summary>設定のGUI。呼び出し側のツリーの中に置くこと(IDが被らないように)</summary>
		void DebugGUI();

		//========================================================================*/
		//* Getter

		/// <summary>ならし終わった結果。両方の段を切ると raw がそのまま返る</summary>
		/// <remarks>Render のたびに実体が変わるので、通した後の値を使うこと</remarks>
		D3D12_GPU_DESCRIPTOR_HANDLE GetResultSrvHandle() const;

		double GetTemporalMS();
		double GetSpatialMS();

	private:

		/// <summary>HLSL 側 (Raytracing/DenoiseTemporal.CS.hlsl) の DenoiseTemporalParam と並びが一致していること</summary>
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

		/// <summary>HLSL 側 (Raytracing/DenoiseSpatial.CS.hlsl) の DenoiseSpatialParam と並びが一致していること</summary>
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
			kTimer_Temporal = 0, // 時間方向の蓄積
			kTimer_Spatial,      // 空間フィルタ(全回数の合計)
			kTimer_Count
		};

		/// <summary>1フレーム分の定数を書き込む</summary>
		void UpdateParam(uint32_t frameIndex, const GBufferPass& gbuffer);

		/// <summary>前フレームまでの蓄積と混ぜる</summary>
		void DispatchTemporal(uint32_t frameIndex, const GBufferPass& gbuffer, const WorkTexture& raw);

		/// <summary>à-trous を1回ぶん、横か縦の片方向だけ掛ける</summary>
		/// <param name="paramSlot">定数バッファの何番目を使うか。方向ごとに別の中身が要る</param>
		void DispatchSpatial(uint32_t frameIndex, const GBufferPass& gbuffer,
			uint32_t iteration, uint32_t paramSlot, bool vertical,
			uint32_t srcSrvIndex, const WorkTexture& dst);

	private:

		DXC::DXCom* dxcommon_ = nullptr;

		// 次フレームへ渡す蓄積。R16G16_FLOAT で x=信号, y=蓄積フレーム数
		WorkTexture history_[DXC::kFrameCount_];
		// à-trous のピンポン。時間方向の出力もここの[0]へ書く
		WorkTexture filter_[2];

		// 読み手が引くもの。設定によって raw にも filter_ にもなる
		uint32_t finalSrvIndex_ = 0;

		// 生成直後に「遮蔽なし」で塗るためだけのRTV。以後は使わない
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> clearRtvHeap_;

		Microsoft::WRL::ComPtr<ID3D12Resource> temporalResource_[DXC::kFrameCount_];
		DenoiseTemporalParam* temporalMapped_[DXC::kFrameCount_] = {};
		// 回数ぶんを1本にまとめ、256バイトずつずらして参照する
		Microsoft::WRL::ComPtr<ID3D12Resource> spatialResource_[DXC::kFrameCount_];
		uint8_t* spatialMapped_[DXC::kFrameCount_] = {};

		// 再投影に使う。カメラは毎フレーム動くので前フレームの行列を持っておく
		Math::Matrix4x4 prevViewProj_;
		Math::Matrix4x4 prevInvViewProj_;
		// 前フレームが蓄積を書いたか。パスを入れ直した直後は履歴が古いので見てはいけない
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
