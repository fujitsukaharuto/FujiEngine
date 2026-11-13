#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/Matrix4x4.h"
#include "Engine/Math/Vector/Vector3.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	class ClusteredLightPass;
	class RaytracingScene;

	/// <summary>
	/// 雨。カメラのまわりに雨粒の筋を描き、近くの影なしのライトの色で光らせる
	/// </summary>
	/// <remarks>雨粒は番号から位置を決める。強さ0の間は何も描かない。
	/// ほかに地面のしぶき・庇などからの雨だれ・遠くの雨のカーテン・走る車の水はねを、それぞれの量が0でなければ描く</remarks>
	class RainRenderer {
	public:
		RainRenderer() = default;
		~RainRenderer() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>雨の強さ (0〜1)。0で描かない</summary>
		void SetAmount(float amount) { amount_ = amount; }
		float GetAmount() const { return amount_; }
		/// <summary>地面のしぶきの量 (0〜1)。雨の強さにも比例する。0で描かない</summary>
		void SetSplash(float splash) { splash_ = splash; }
		/// <summary>庇・看板・電線などから垂れる雨だれの量 (0〜1)。0で描かない</summary>
		void SetDrip(float drip) { drip_ = drip; }
		/// <summary>遠くの雨のカーテンの濃さ (0〜1)。0で描かない</summary>
		void SetCurtain(float curtain) { curtain_ = curtain; }
		/// <summary>走る車の水はねの量 (0〜1)。0で描かない</summary>
		void SetSpray(float spray) { spray_ = spray; }
		/// <summary>水はねを出すタイヤをこのフレームの分だけ足す</summary>
		/// <param name="contact">タイヤの接地点</param>
		/// <param name="velocity">車の速さ (m/s)</param>
		void AddSprayWheel(const Math::Vector3& contact, float width, const Math::Vector3& velocity);
		/// <summary>水はねを出すか。出さないなら AddSprayWheel を呼ばなくてよい</summary>
		bool IsSprayActive() const { return amount_ > 0.0f && spray_ > 0.0f; }

		/// <summary>このフレームの値を上げ、雨だれの場所を探す</summary>
		/// <remarks>TLAS を組んだ後、前方描画より前に呼ぶ</remarks>
		void Prepare(const RaytracingScene* scene);

		/// <summary>不透明と半透明の後に加算で重ねる</summary>
		void Render(const ClusteredLightPass& clusteredLights, const RaytracingScene* scene);

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
			float splashCenter[2] = {};
			float splashArea = 0.0f;
			float splashBrightness = 0.0f;
			Math::Matrix4x4 inverseViewProjection;
			int32_t dripGridBase[2] = {};
			float dripCellSize = 0.0f;
			uint32_t dripFrame = 0;
			float dripBrightness = 0.0f;
			float dripMaxFall = 0.0f;
			float curtainBrightness = 0.0f;
			float curtainStart = 0.0f;
			float projectionZScale = 0.0f;
			float projectionZOffset = 0.0f;
			uint32_t sprayWheelCount = 0;
			float sprayBrightness = 0.0f;
			float dripAmount = 0.0f;
			float pad[3] = {};
		};
		static_assert(sizeof(RainParams) == 272, "RainParams の大きさが Rain.hlsli と一致していない");

		/// <summary>強さ1のときの雨粒の数</summary>
		static constexpr uint32_t kMaxDrops_ = 40000;
		/// <summary>雨の強さ1・しぶき1のときのしぶきの数</summary>
		static constexpr uint32_t kMaxSplashes_ = 12000;
		/// <summary>しぶき1つの頂点数 (水柱1つと水滴4つ × 6)</summary>
		static constexpr uint32_t kSplashVertices_ = 30;

		/// <summary>Particle/Rain.hlsli の RainDripSite と並びが一致していること</summary>
		struct DripSite {
			float site[4];
			int32_t cell[2];
			float seed;
			uint32_t state;
		};
		static_assert(sizeof(DripSite) == 32, "DripSite の大きさが Rain.hlsli と一致していない");
		/// <summary>雨だれを探す格子の1辺のマス数。RainDripFind.CS の kDripGridSize と同じ値</summary>
		static constexpr uint32_t kDripGridSize_ = 128;
		/// <summary>雨だれ1か所の頂点数 (滴2つ × 6)</summary>
		static constexpr uint32_t kDripVertices_ = 12;

		/// <summary>Particle/Rain.hlsli の RainSprayWheel と並びが一致していること</summary>
		struct SprayWheel {
			float position[4];
			float velocity[4];
		};
		static_assert(sizeof(SprayWheel) == 32, "SprayWheel の大きさが Rain.hlsli と一致していない");
		static constexpr uint32_t kMaxSprayWheels_ = 128;
		/// <summary>タイヤ1つの粒の数。RainSpray.VS の kSprayParticles と同じ値</summary>
		static constexpr uint32_t kSprayParticles_ = 96;
		/// <summary>遠くの雨のカーテンの層の数。RainCurtain.hlsli の kRainCurtainLayers と同じ値</summary>
		static constexpr uint32_t kCurtainLayers_ = 4;

		void BindLights(const ClusteredLightPass& clusteredLights) const;

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		RainParams* paramMapped_[DXC::kFrameCount_] = {};

		// 雨だれの場所。フレームをまたいで取っておく
		Microsoft::WRL::ComPtr<ID3D12Resource> dripSiteResource_;
		uint32_t dripSiteSrvIndex_ = 0;
		uint32_t dripSiteUavIndex_ = 0;
		bool hasDripSites_ = false;
		uint32_t dripFrame_ = 0;

		// 水はねを出すタイヤ (毎フレーム CPU から書く)
		Microsoft::WRL::ComPtr<ID3D12Resource> sprayResource_[DXC::kFrameCount_];
		SprayWheel* sprayMapped_[DXC::kFrameCount_] = {};
		uint32_t spraySrvIndex_[DXC::kFrameCount_] = {};
		std::vector<SprayWheel> sprayWheels_;
		uint32_t sprayWheelCount_ = 0;

		float amount_ = 0.0f;
		float splash_ = 0.0f;
		float drip_ = 0.0f;
		float curtain_ = 0.0f;
		float spray_ = 0.0f;
		float time_ = 0.0f;
		// 見た目の調整 (保存しない)
		float boxSize_ = 36.0f;
		float fallSpeed_ = 11.0f;
		float streakLength_ = 0.55f;
		float width_ = 0.008f;
		float brightness_ = 0.3f;
		float wind_[2] = { 0.8f,0.3f };
		float splashArea_ = 24.0f;
		float splashBrightness_ = 2.0f;
		float dripCellSize_ = 0.35f;
		float dripMaxFall_ = 10.0f;
		float dripBrightness_ = 2.5f;
		float curtainBrightness_ = 0.6f;
		float curtainStart_ = 16.0f;
		float sprayBrightness_ = 3.0f;
	};

}
