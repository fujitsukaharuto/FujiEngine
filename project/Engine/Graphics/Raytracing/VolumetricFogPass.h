#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/Matrix4x4.h"
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector3.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class Camera;
	class ClusteredLightPass;
	class LightManager;
	class RaytracingScene;

	/// <summary>
	/// ボリュメトリックフォグ (UE の Volumetric Fog / Hillaire 2015)
	/// </summary>
	/// <remarks>
	/// カメラの前を画面 8px 角 × 奥行き64段の箱に分け、箱ごとに霧の濃さと受ける光 (影のレイ込み) を求めて
	/// 前のフレームと混ぜ、手前から積んだ3Dの表を作る。前方描画・空・映り込みのパスが引く。
	/// 霧の濃さは配置データの高さの霧と同じ式。表の届く奥行きより先は式の霧のまま
	/// </remarks>
	class VolumetricFogPass {
	public:
		/// <summary>配置データの環境が入れる設定。名前は UE の Exponential Height Fog に合わせてある</summary>
		struct Settings {
			bool enabled = false;
			/// <summary>散乱 / 消散。1 で霧が光を吸わない</summary>
			Math::Vector3 albedo = { 1.0f,1.0f,1.0f };
			/// <summary>霧が自分で出す光 (1mあたり)</summary>
			Math::Vector3 emissive = { 0.0f,0.0f,0.0f };
			/// <summary>霧の濃さに掛ける倍率</summary>
			float extinctionScale = 1.0f;
			/// <summary>散乱の向きの偏り (UE の Scattering Distribution)。0 で全方向へ同じ</summary>
			float scatteringDistribution = 0.2f;
			/// <summary>表が届く奥行き (UE の View Distance)</summary>
			float viewDistance = 60.0f;
			/// <summary>平行光源・空・ライトの光の倍率 (UE の Volumetric Scattering Intensity)</summary>
			float sunScale = 1.0f;
			float skyLightScale = 1.0f;
			float localLightScale = 1.0f;
			/// <summary>前のフレームと混ぜて、箱の中のずらしを均す</summary>
			bool temporal = true;
		};

		/// <summary>局所の霧 (UE の Local Fog Volume) 1つ。球を拡縮・回転した楕円体</summary>
		struct LocalFogVolumeDesc {
			Math::Vector3 center{};
			/// <summary>楕円体の半径 (m)</summary>
			Math::Vector3 radii = { 5.0f,5.0f,5.0f };
			/// <summary>回転 (ラジアン)</summary>
			Math::Vector3 rotation{};
			/// <summary>中心の濃さ (1mあたり)。縁へ (1 - r^2) で薄まる</summary>
			float radialExtinction = 0.2f;
			/// <summary>楕円体の中の高さの霧の濃さ (1mあたり)。基準の高さでこの値</summary>
			float heightExtinction = 0.0f;
			/// <summary>高さの霧の薄まり方 (1mあたり)</summary>
			float heightFalloff = 1.0f;
			/// <summary>高さの霧の基準の高さ。中心からのずれ (m)</summary>
			float heightOffset = 0.0f;
			Math::Vector3 albedo = { 1.0f,1.0f,1.0f };
			/// <summary>霧が見せる自分の光。濃い所ほど強く見える</summary>
			Math::Vector3 emissive{};
			/// <summary>ボリュメトリックフォグを切っているときの散乱の向きの偏り</summary>
			float phaseG = 0.2f;
		};
		/// <summary>局所の霧を置ける数</summary>
		static constexpr uint32_t kMaxLocalFogVolumes = 16;

		VolumetricFogPass() = default;
		~VolumetricFogPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		void SetSettings(const Settings& settings) { settings_ = settings; }
		const Settings& GetSettings() const { return settings_; }
		/// <summary>局所の霧を入れる。多ければ kMaxLocalFogVolumes までで切る</summary>
		/// <remarks>表を作らない設定でも、前方描画と空が式で掛ける</remarks>
		void SetLocalFogVolumes(const std::vector<LocalFogVolumeDesc>& volumes);

		/// <summary>このフレームの表を作る。作らないフレームも毎フレーム呼ぶ (定数に enabled=0 を書く)</summary>
		/// <remarks>TLAS と影なしのライトの振り分けが済んでいて、空と前方描画より前であること</remarks>
		void Render(const RaytracingScene* scene, const ClusteredLightPass* clustered, const Camera& camera);

		/// <summary>このフレームに表を作ったか</summary>
		bool IsRendered() const { return isRendered_; }
		/// <summary>このフレームの局所の霧の数</summary>
		uint32_t GetLocalFogVolumeCount() const { return localFogVolumeCount_; }

		/// <summary>このフレームの定数。前方描画・空・映り込みのパスが引く</summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetParamsGPUVirtualAddress() const;
		/// <summary>積んだ表。rgb = 霧が散らした光、a = 通り抜ける割合</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetVolumeSrvHandle() const;
		/// <summary>このフレームの局所の霧 (奥から手前の順)</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetLocalFogVolumesSrvHandle() const;

		void DebugGUI();

	private:

		/// <summary>HLSL 側 (Common/VolumetricFog.hlsli) の VolumetricFogParams と並びが一致していること</summary>
		struct VolumetricFogParams {
			Math::Matrix4x4 invViewProj;
			Math::Matrix4x4 prevViewProj;
			Math::Vector3 cameraPosition{};
			uint32_t enabled = 0;
			Math::Vector3 cameraForward{};
			float maxDistance = 0.0f;
			Math::Vector3 gridZParams{};
			float historyWeight = 0.0f;
			uint32_t gridSize[3] = {};
			uint32_t padGrid = 0;
			Math::Vector2 screenToGridUV{};
			Math::Vector2 invScreenSize{};
			Math::Vector3 jitter{};
			float phaseG = 0.0f;
			Math::Vector3 albedo{};
			float extinctionScale = 1.0f;
			Math::Vector3 emissive{};
			float sunScale = 1.0f;
			float skyLightScale = 1.0f;
			float localLightScale = 1.0f;
			float padScale[2] = {};
			Math::Vector3 lightDirection = { 0.0f,1.0f,0.0f };
			uint32_t localFogVolumeCount = 0;
			Math::Vector3 lightRadiance{};
			float padLight = 0.0f;
			Math::Vector3 ambientSky{};
			float padSky = 0.0f;
			Math::Vector3 ambientGround{};
			float padGround = 0.0f;
		};
		static_assert(sizeof(VolumetricFogParams) == 336, "VolumetricFogParams の大きさが VolumetricFog.hlsli と一致していない");

		/// <summary>HLSL 側 (Common/VolumetricFog.hlsli) の LocalFogVolume と並びが一致していること</summary>
		struct LocalFogVolumeGPU {
			Math::Matrix4x4 worldToUnit;
			Math::Vector3 albedo{};
			float radialExtinction = 0.0f;
			Math::Vector3 emissive{};
			float heightExtinction = 0.0f;
			float heightFalloff = 0.0f;
			float baseHeight = 0.0f;
			float phaseG = 0.0f;
			float padVolume = 0.0f;
		};
		static_assert(sizeof(LocalFogVolumeGPU) == 112, "LocalFogVolumeGPU の大きさが VolumetricFog.hlsli と一致していない");

		// 箱の大きさ。画面のこの画素数の角を1つの箱にする (UE の r.VolumetricFog.GridPixelSize)
		static constexpr uint32_t kGridPixelSize_ = 8;
		// 奥行きの段の数 (UE の r.VolumetricFog.GridSizeZ)
		static constexpr uint32_t kGridSizeZ_ = 64;
		// 奥行きの段の並べ方 (UE の r.VolumetricFog.DepthDistributionScale)。大きいほど等間隔に近い
		static constexpr float kDepthDistributionScale_ = 32.0f;
		// 前のフレームを混ぜる割合 (UE の r.VolumetricFog.HistoryWeight)
		static constexpr float kHistoryWeight_ = 0.9f;
		static constexpr uint32_t kThreadGroupSize_ = 8;
		// ルートCBVのアドレスは256バイト境界
		static constexpr uint64_t kParamStride_ = 512;

		enum TimerID : uint32_t {
			kTimer_Scatter = 0,
			kTimer_Integrate,
			kTimer_Count,
		};

		struct VolumeTexture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
		};

		VolumeTexture CreateVolume(ID3D12Device* device);
		/// <summary>段の深さの並びを決める係数。最初の段を near、最後の段の中心を far に置く</summary>
		static Math::Vector3 MakeGridZParams(float nearDepth, float farDepth);
		/// <summary>局所の霧を奥から手前の順にこのフレームのバッファへ書く</summary>
		/// <returns>書いた数</returns>
		uint32_t UploadLocalFogVolumes(uint32_t frameIndex, const Math::Vector3& cameraPosition);
		/// <summary>表を作れる状態か</summary>
		bool CanRender(const RaytracingScene* scene, const ClusteredLightPass* clustered) const;

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		Settings settings_;
		bool isRendered_ = false;
		uint32_t localFogVolumeCount_ = 0;

		uint32_t gridWidth_ = 0;
		uint32_t gridHeight_ = 0;

		// 箱ごとの値。前のフレームのものを履歴として読みながら、もう一方へ書く
		VolumeTexture scatter_[2];
		uint32_t scatterIndex_ = 0;
		// 手前から積んだ表
		VolumeTexture integrated_;

		// 局所の霧。フレームごとに書く
		std::vector<LocalFogVolumeDesc> localFogVolumes_;
		Microsoft::WRL::ComPtr<ID3D12Resource> localFogResource_[DXC::kFrameCount_];
		LocalFogVolumeGPU* localFogMapped_[DXC::kFrameCount_] = {};
		uint32_t localFogSrvIndex_[DXC::kFrameCount_] = {};

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_;
		uint8_t* paramMapped_ = nullptr;

		// 履歴が使えるか。作らなかったフレームや段の並びが変わったら捨てる
		bool hasHistory_ = false;
		Math::Matrix4x4 prevViewProj_;
		Math::Vector3 prevGridZParams_{};
		uint32_t jitterIndex_ = 0;

		DXC::GPUTimer gpuTimer_;
	};

}
