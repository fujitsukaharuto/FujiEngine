#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// 多段のブルーム。明るい所を半分ずつ縮めながらぼかし、小さい方から重ねて戻してシーンへ足す
	/// </summary>
	/// <remarks>ポストエフェクトの並びの1つとして OffscreenManager から呼ばれる</remarks>
	class BloomPass {
	public:
		BloomPass() = default;
		~BloomPass() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>シーンにブルームを足したものを出力へ書く</summary>
		/// <param name="sceneSrv">元のシーン。読み取り状態(NON_PIXEL_SHADER_RESOURCE)であること</param>
		/// <param name="outputUav">書き込み先。画面と同じ大きさで、UAV状態であること</param>
		/// <param name="threshold">これより明るい所が光る</param>
		/// <param name="intensity">シーンへ足すときの強さ</param>
		void Render(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE outputUav,
			float threshold, float intensity);

		/// <summary>しきい値の前後をなだらかにする幅と、光の広がり</summary>
		void SetShape(float knee, float scatter) { knee_ = knee; scatter_ = scatter; }

		/// <summary>かかった時間の表示。呼び出し側のツリーの中に置くこと</summary>
		void DebugGUI();

	private:

		/// <summary>HLSL 側 (PostEffect/BloomPass.hlsli) の BloomPassParam と並びが一致していること</summary>
		struct BloomPassParam {
			float srcTexelSize[2]{};
			uint32_t dstSize[2]{};
			float threshold = 0.0f;
			float knee = 0.0f;
			float scatter = 0.0f;
			float intensity = 0.0f;
			uint32_t prefilter = 0;
			float pad[3]{};
		};

		/// <summary>縮小していく段数。1段ごとに半分になるので、6段で画面の 1/64 まで下りる</summary>
		static constexpr uint32_t kMipCount_ = 6;

		// ディスパッチごとに定数が違う。縮小が段数ぶん、拡大が段数-1、合成が1
		static constexpr uint32_t kParamSlotCount_ = kMipCount_ * 2;
		// ルートCBVのアドレスは256バイト境界に載っている必要がある
		static constexpr uint64_t kParamStride_ = 256;

		/// <summary>1段ぶんのテクスチャ</summary>
		struct Mip {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
			uint32_t width = 0;
			uint32_t height = 0;
		};

		enum TimerId : uint32_t {
			kTimer_Bloom = 0,
			kTimer_Count
		};

		void CreateMip(Mip& mip, uint32_t width, uint32_t height);

		/// <summary>定数を1枠ぶん書き込み、そのアドレスを返す</summary>
		D3D12_GPU_VIRTUAL_ADDRESS WriteParam(uint32_t frameIndex, uint32_t slot, const BloomPassParam& param);

	private:

		DXC::DXCom* dxcommon_ = nullptr;

		// 同じフレームの中で書いて読み切るので、フレーム数ぶんは要らない
		Mip down_[kMipCount_];
		// 拡大は「1段小さい拡大結果」と「同じ大きさの縮小結果」を読んで書くので、縮小とは別に持つ
		Mip up_[kMipCount_ - 1];

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		uint8_t* paramMapped_[DXC::kFrameCount_] = {};

		float knee_ = 0.1f;
		float scatter_ = 0.7f;

		DXC::GPUTimer gpuTimer_;
	};

}
