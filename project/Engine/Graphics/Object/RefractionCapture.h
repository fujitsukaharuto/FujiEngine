#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Graphics/Raytracing/WorkTexture.h"
#include "Engine/Math/Matrix/Matrix4x4.h"

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// 半透明の屈折。不透明の物を描き終えた画面を写し、半透明の物がずらして引く
	/// </summary>
	/// <remarks>画面に写っていない物は映らない。強さが0か半透明の物が無いフレームは写さない</remarks>
	class RefractionCapture {
	public:
		RefractionCapture() = default;
		~RefractionCapture() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>屈折の強さ。面の奥のこの距離(m)の物を引く。0で切る</summary>
		void SetStrength(float strength) { strength_ = strength; }
		bool IsEnabled() const { return strength_ > 0.0f; }

		/// <summary>このフレームの定数を書く。描き始める前に1度だけ呼ぶ</summary>
		/// <param name="willCapture">このフレームに Capture を呼ぶか</param>
		void Prepare(bool willCapture);

		/// <summary>不透明の物を描き終えた画面を写す。レンダーターゲットはそのまま</summary>
		void Capture();

		/// <summary>前方描画の PS へ写しと定数を渡す。切っていても要る</summary>
		void Bind(ID3D12GraphicsCommandList* commandList) const;

	private:

		/// <summary>HLSL 側 (Common/Refraction.hlsli) の RefractionParams と並びが一致していること</summary>
		struct RefractionParams {
			Math::Matrix4x4 viewProjection;
			float strength = 0.0f;
			float ior = 1.5f;	// 屈折率
			uint32_t enabled = 0;
			float pad = 0.0f;
		};

		DXC::DXCom* dxcommon_ = nullptr;
		float strength_ = 0.0f;

		WorkTexture scene_;
		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		RefractionParams* paramMapped_[DXC::kFrameCount_] = {};
	};

}
