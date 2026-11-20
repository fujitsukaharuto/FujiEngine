#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/Matrix4x4.h"
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector3.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class Camera;

	/// <summary>
	/// 太陽の向きから空を計算する (Hillaire 2020 / UE の Sky Atmosphere)
	/// </summary>
	/// <remarks>透過率と多重散乱は最初に1回、SkyView・キューブマップ・空気遠近の3Dの表は毎フレーム作る</remarks>
	class SkyAtmosphere {
	public:
		/// <summary>大気の性質。長さは km</summary>
		/// <remarks>HLSL 側 (AtmosphereModel.hlsli の AtmosphereParams) と並びが一致していること</remarks>
		struct AtmosphereParams {
			float bottomRadius = 6360.0f;
			float topRadius = 6460.0f;
			float rayleighDensityScale = -1.0f / 8.0f;
			float mieDensityScale = -1.0f / 1.2f;

			Math::Vector3 rayleighScattering = { 5.802e-3f, 13.558e-3f, 33.1e-3f };
			float miePhaseG = 0.8f;

			Math::Vector3 mieScattering = { 3.996e-3f, 3.996e-3f, 3.996e-3f };
			float ozoneCenter = 25.0f;

			Math::Vector3 mieExtinction = { 4.440e-3f, 4.440e-3f, 4.440e-3f };
			float ozoneWidth = 15.0f;

			Math::Vector3 ozoneAbsorption = { 0.650e-3f, 1.881e-3f, 0.085e-3f };
			float cameraHeight = 0.0f;

			Math::Vector3 groundAlbedo = { 0.3f, 0.3f, 0.3f };
			float cloudCoverage = 0.0f;	// 雲の量。Render が入れる

			Math::Vector3 sunDirection = { 0.0f, 1.0f, 0.0f };
			float cloudTime = 0.0f;		// 雲を流す時刻(秒)。Render が入れる

			Math::Vector3 sunIlluminance = { 1.0f, 1.0f, 1.0f };
			float pad2 = 0.0f;
		};
		static_assert(sizeof(AtmosphereParams) == 128, "AtmosphereParams の大きさが AtmosphereModel.hlsli と一致していない");

		/// <summary>CPU へ読み戻す値。GPU が書いてから数フレーム遅れて届く</summary>
		struct Summary {
			// 地平線の少し上の色。+X, -X, +Z, -Z の順
			Math::Vector3 horizon[4]{};
			// 上向きと下向きの面に届く光の平均放射輝度
			Math::Vector3 ambientUp{};
			Math::Vector3 ambientDown{};
			// カメラから太陽への透過率
			Math::Vector3 sunTransmittance{};
			// カメラの位置の多重散乱 (太陽の明るさ1あたり)
			Math::Vector3 multiScattering{};
		};

		/// <summary>その高さの空気の係数。1m あたり</summary>
		struct Medium {
			Math::Vector3 rayleigh{};
			Math::Vector3 mie{};
			Math::Vector3 extinction{};
		};

		/// <summary>空気遠近の3Dの表を作る側と引く側の定数</summary>
		/// <remarks>HLSL 側 (Common/AerialPerspective.hlsli) と並びが一致していること</remarks>
		struct AerialPerspectiveParams {
			Math::Matrix4x4 invViewProj;
			Math::Vector3 sunIlluminance{};	// 太陽の色と強さ × 空に掛けている色
			uint32_t enabled = 0;			// 0 ならこのフレームは表を作っていない
			Math::Vector2 invScreenSize{};
			float invMaxDistance = 0.0f;	// 1 / 表の届く距離 (m)
			float maxDistanceKm = 0.0f;		// 表の届く距離 × 空気遠近の倍率 (km)
		};
		static_assert(sizeof(AerialPerspectiveParams) == 96, "AerialPerspectiveParams の大きさが AerialPerspective.hlsli と一致していない");

		SkyAtmosphere() = default;
		~SkyAtmosphere() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>星と大気の大きさと、空から見たカメラの高さを入れる</summary>
		/// <param name="altitudeKm">カメラの高さに足す高度 (km)。大きくすると宇宙から星を見下ろす</param>
		/// <param name="scale">星と大気の大きさの倍率。散乱の係数は逆に掛けるので、地表から見た空の色は変わらない</param>
		/// <remarks>大きさが変わったら透過率と多重散乱を作り直し、IBL を焼き直す</remarks>
		void SetPlanet(float altitudeKm, float scale);
		/// <summary>空から見たカメラの高度 (km)。ワールドの高さ (m) に SetPlanet の高度を足したもの</summary>
		float GetCameraAltitudeKm(float cameraY) const;
		/// <summary>カメラから見た地面の縁の天頂角。SkyView の数表の並び (AtmosphereModel.hlsli の AtmosphereHorizonZenith と同じ式)</summary>
		float GetHorizonZenith(float cameraY) const;

		/// <summary>このフレームの空の数表とキューブマップを作り、CPU へ読み戻す値を書き出す</summary>
		/// <remarks>描画のコマンドリストが開いている間に、空を描くより前に呼ぶ</remarks>
		/// <param name="sunDirection">太陽へ向かう単位ベクトル</param>
		/// <param name="sunIlluminance">太陽の色と強さ</param>
		/// <param name="cameraHeight">カメラのワールドの高さ(m)。SetPlanet の高度を足して使う</param>
		/// <param name="cloudCoverage">雲の量。0で雲を出さない</param>
		/// <param name="cloudTime">雲を流す時刻(秒)</param>
		void Render(ID3D12GraphicsCommandList* commandList, const Math::Vector3& sunDirection,
			const Math::Vector3& sunIlluminance, float cameraHeight, float cloudCoverage, float cloudTime);

		/// <summary>空気遠近の3Dの表 (UE の Aerial Perspective LUT) を作る</summary>
		/// <remarks>作らないフレームも毎フレーム呼ぶ。作るときは同じフレームの Render の後</remarks>
		/// <param name="enabled">表を作るか</param>
		/// <param name="distanceScale">空気遠近の距離の倍率</param>
		/// <param name="tint">空に掛けている色</param>
		void RenderAerialPerspective(ID3D12GraphicsCommandList* commandList, const Camera& camera, bool enabled,
			float distanceScale, const Math::Vector3& tint);

		/// <summary>GPU が書き終えた値を読む。その番号で Render していなければ false</summary>
		/// <remarks>その番号の GPU の完了を待った後に呼ぶ</remarks>
		bool ReadSummary(uint32_t frameIndex, Summary& summary);

		/// <summary>その高さ(m)の空気の係数。SetPlanet の高度を足して使う</summary>
		Medium GetMedium(float height) const;

		void DebugGUI();

		//========================================================================*/
		//* Getter

		const AtmosphereParams& GetParams() const { return params_; }
		/// <summary>カメラから見た空の数表。空の描画が引く</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetSkyViewSrvHandle() const;
		/// <summary>空を写したキューブマップ。太陽の円盤は入っていない</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetSkyCubeSrvHandle() const;
		uint32_t GetSkyCubeSize() const { return kSkyCubeSize_; }
		/// <summary>キューブマップが変わると増える。IBL を焼き直す合図</summary>
		uint64_t GetSkyCubeVersion() const { return skyCubeVersion_; }
		/// <summary>一度でも作ったか</summary>
		bool IsReady() const { return isStaticLutReady_; }
		/// <summary>このフレームの空気遠近の定数</summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetAerialPerspectiveParamsAddress() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetAerialScatterSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetAerialTransmittanceSrvHandle() const;

	private:

		// 数表の大きさ。HLSL 側の定数と一致していること
		static constexpr uint32_t kTransmittanceWidth_ = 256;
		static constexpr uint32_t kTransmittanceHeight_ = 64;
		static constexpr uint32_t kMultiScatterSize_ = 32;
		static constexpr uint32_t kSkyViewWidth_ = 192;
		static constexpr uint32_t kSkyViewHeight_ = 108;
		// ミップは IBL を焼くときに引く
		static constexpr uint32_t kSkyCubeSize_ = 64;
		static constexpr uint32_t kSkyCubeMipLevels_ = 7;
		// 空気遠近の表。縦横は画面、奥行きは距離の平方根で区切る
		static constexpr uint32_t kAerialLutWidth_ = 32;
		static constexpr uint32_t kAerialLutHeight_ = 32;
		static constexpr uint32_t kAerialLutDepth_ = 32;

		/// <summary>並びは AtmosphereSummary.CS.hlsl と一致していること</summary>
		enum SummaryIndex : uint32_t {
			kSummaryHorizon = 0,
			kSummaryAmbientUp = 4,
			kSummaryAmbientDown = 5,
			kSummarySunTransmittance = 6,
			kSummaryMultiScattering = 7,
			kSummaryCount = 8,
		};
		static constexpr size_t kSummaryBytes_ = sizeof(float) * 4 * kSummaryCount;

		// ルートCBVのアドレスは256バイト境界
		static constexpr uint64_t kParamStride_ = 256;

		/// <summary>太陽がこれだけ動くか明るさが変わったら IBL を焼き直す</summary>
		static constexpr float kRebakeAngleCos_ = 0.99996f; // 約0.5度
		static constexpr float kRebakeIlluminanceRatio_ = 0.02f;
		/// <summary>焼き直しの最短の間隔</summary>
		static constexpr uint32_t kRebakeIntervalFrames_ = 8;

		struct LutTexture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
		};

		void CreateResources(ID3D12Device* device);
		LutTexture CreateLut(ID3D12Device* device, uint32_t width, uint32_t height);
		LutTexture CreateVolumeLut(ID3D12Device* device);
		/// <summary>透過率と多重散乱。大気の性質だけで決まる</summary>
		void RenderStaticLuts(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS params);
		/// <summary>IBL を焼き直すほど太陽が変わったか</summary>
		void UpdateSkyCubeVersion(const Math::Vector3& sunDirection, const Math::Vector3& sunIlluminance);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		AtmosphereParams params_;

		LutTexture transmittance_;
		LutTexture multiScatter_;
		LutTexture skyView_;

		Microsoft::WRL::ComPtr<ID3D12Resource> skyCube_;
		uint32_t skyCubeSrvIndex_ = 0;
		uint32_t skyCubeUavIndex_[kSkyCubeMipLevels_] = {};

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_;
		uint8_t* paramMapped_ = nullptr;

		LutTexture aerialScatter_;
		LutTexture aerialTransmittance_;
		Microsoft::WRL::ComPtr<ID3D12Resource> aerialParamResource_;
		uint8_t* aerialParamMapped_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> summaryBuffer_;
		uint32_t summaryUavIndex_ = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> summaryReadback_[DXC::kFrameCount_];
		const float* summaryMapped_[DXC::kFrameCount_] = {};
		// その番号のフレームで書き出しを積んだか。読んだら下ろす
		bool isSummaryPending_[DXC::kFrameCount_] = {};

		bool isStaticLutReady_ = false;

		uint64_t skyCubeVersion_ = 0;
		// 次の Render で必ず版を上げる
		bool isRebakeForced_ = true;
		Math::Vector3 bakedSunDirection_{};
		Math::Vector3 bakedSunIlluminance_{};
		// 焼いたときのカメラの高度 (km)。空の見え方が変わるほど動いたら焼き直す
		float bakedCameraHeight_ = 0.0f;
		// SetPlanet の値
		float altitudeKm_ = 0.0f;
		float planetScale_ = 1.0f;
		uint32_t framesSinceRebake_ = 0;
	};

}
