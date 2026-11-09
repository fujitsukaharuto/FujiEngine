#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/Matrix4x4.h"
#include "Engine/Math/Vector/Vector3.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	class ClusteredLightPass;

	/// <summary>
	/// 雨。カメラのまわりに雨粒の筋を描き、近くの影なしのライトの色で光らせる
	/// </summary>
	/// <remarks>雨粒は番号から位置を決める。強さ0の間は何も描かない</remarks>
	class RainRenderer {
	public:
		RainRenderer() = default;
		~RainRenderer() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>雨の強さ (0〜1)。0で描かない</summary>
		void SetAmount(float amount) { amount_ = amount; }
		float GetAmount() const { return amount_; }

		/// <summary>不透明と半透明の後に加算で重ねる</summary>
		void Render(const ClusteredLightPass& clusteredLights);

		void DebugGUI();

	private:

		/// <summary>Particle/Rain.hlsli の RainParams と並びが一致していること</summary>
		struct RainParams {
			Math::Matrix4x4 viewProjection;
			Math::Vector3 cameraPosition;
			float time = 0.0f;
			Math::Vector3 ambient;
			float boxSize = 0.0f;
			float wind[2] = {};
			float fallSpeed = 0.0f;
			float streakLength = 0.0f;
			float width = 0.0f;
			float brightness = 0.0f;
			float screenSize[2] = {};
		};
		static_assert(sizeof(RainParams) == 128, "RainParams の大きさが Rain.hlsli と一致していない");

		/// <summary>強さ1のときの雨粒の数</summary>
		static constexpr uint32_t kMaxDrops_ = 40000;

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		RainParams* paramMapped_[DXC::kFrameCount_] = {};

		float amount_ = 0.0f;
		float time_ = 0.0f;
		// 見た目の調整 (保存しない)
		float boxSize_ = 36.0f;
		float fallSpeed_ = 11.0f;
		float streakLength_ = 0.55f;
		float width_ = 0.008f;
		float brightness_ = 0.3f;
		float wind_[2] = { 0.8f,0.3f };
	};

}
