#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <random>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Vector/Vector3.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	class RaytracingScene;

	/// <summary>プローブの格子1つぶんの定数。DDGIPass と BakedGIPass で共有する</summary>
	/// <remarks>HLSL 側 (Common/DDGICommon.hlsli の DDGIVolume) と並びが一致していること</remarks>
	struct DDGIVolumeData {
		int32_t baseCell[3] = {};
		float spacing = 3.0f;
		int32_t previousBaseCell[3] = {};
		uint32_t raysPerProbe = 64;
		int32_t probeCounts[3] = {};
		float hysteresis = 0.97f;
		float rayRotation[3][4] = { { 1.0f,0.0f,0.0f,0.0f },{ 0.0f,1.0f,0.0f,0.0f },{ 0.0f,0.0f,1.0f,0.0f } };
		float normalBias = 0.25f;
		float viewBias = 0.75f;
		float maxRayDistance = 200.0f;
		uint32_t resetAll = 1;
		float irradianceAtlasSize[2] = {};
		float distanceAtlasSize[2] = {};
		uint32_t probeStride = 1;
		uint32_t probeOffset = 0;
		uint32_t firstProbe = 0;   // レイの結果でこの格子が始まる行
		uint32_t firstTileRow = 0; // アトラスでこの格子が始まるタイルの行
		uint32_t bakeComponent = 0;        // 0 なら毎フレームの格子
		uint32_t irradianceRowOffset = 0;  // 放射照度のアトラスだけ、タイルの行に足す量
		uint32_t bakeFlags = 0;
		float bakePad = 0.0f;
		float bakeToSun[3] = {};           // 太陽の成分を焼くときの太陽への向き
		float bakeSunPad = 0.0f;
	};
	static_assert(sizeof(DDGIVolumeData) == 176, "DDGIVolumeData の大きさが DDGICommon.hlsli の DDGIVolume と一致していない");

	/// <summary>
	/// プローブによる間接光 (DDGI)
	/// </summary>
	/// <remarks>格子はカメラについて動き、粗いものから細かいものまで3段重ねる</remarks>
	class DDGIPass {
	public:
		/// <summary>Environment から入る設定</summary>
		struct Settings {
			/// <summary>いちばん粗い格子のプローブの間隔(m)</summary>
			float probeSpacing = 3.0f;
			/// <summary>プローブ1つあたりのレイの本数</summary>
			uint32_t raysPerProbe = 64;
		};

		DDGIPass() = default;
		~DDGIPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームのレイを飛ばしてプローブを更新する</summary>
		/// <remarks>粗い格子から先に更新する。TLAS を組んだ後、前方描画より前に呼ぶ</remarks>
		void Render(RaytracingScene& scene);

		void SetSettings(const Settings& settings);

		void DebugGUI();

		//========================================================================*/
		//* Getter

		/// <summary>引く側に渡す、全部の格子の定数 (DDGI.hlsli の gDDGIVolumes)</summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetVolumesGPUVirtualAddress() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetIrradianceSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetDistanceSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetProbeDataSrvHandle() const;

	private:

		// 格子の数。0 がいちばん粗い。HLSL 側の kDDGIVolumeCount と一致していること
		static constexpr uint32_t kVolumeCount_ = 3;
		// 各格子の間隔は probeSpacing のこの割合
		static constexpr float kSpacingRatios_[kVolumeCount_] = { 1.0f, 1.0f / 3.0f, 1.0f / 6.0f };
		// 各軸のプローブの数。細かい格子は床と天井が両方入るよう縦にも同じ数
		static constexpr int32_t kProbeCounts_[kVolumeCount_][3] = { { 16, 8, 16 }, { 12, 12, 12 }, { 12, 12, 12 } };
		// カメラより下に置くセルの数
		static constexpr int32_t kCellsBelow_[kVolumeCount_] = { 6, 7, 7 };
		static constexpr uint32_t ProbeCount(uint32_t volumeIndex) {
			return static_cast<uint32_t>(kProbeCounts_[volumeIndex][0] * kProbeCounts_[volumeIndex][1] * kProbeCounts_[volumeIndex][2]);
		}
		// HLSL 側 (Common/DDGICommon.hlsli) の定数と一致していること
		static constexpr uint32_t kIrradianceTexels_ = 8;
		static constexpr uint32_t kDistanceTexels_ = 16;
		static constexpr uint32_t kMaxRaysPerProbe_ = 128;
		// 向きを回さないレイ。埋まっているかの判定と再配置に使う
		static constexpr uint32_t kFixedRays_ = 32;
		static constexpr uint32_t kRayThreadGroup_ = 32;

		// 前のフレームの値を残す割合 (1フレームおきの更新)
		static constexpr float kHysteresis_ = 0.97f;
		// 何個おきに更新するか。格子が動いたフレームと作り直すフレームは全部
		static constexpr uint32_t kProbeUpdateStride_ = 2;
		static constexpr float kMaxRayDistance_ = 200.0f;
		// 格子の真ん中をカメラの前方へずらす量 (横幅に対する割合)
		static constexpr float kForwardShift_ = 0.35f;
		// 置きたい位置がこのセル数以内なら格子を動かさない
		static constexpr int32_t kGridSlackCells_ = 2;

		// 1フレームぶんの定数の並び: 格子ごと → 全部まとめたもの
		static constexpr uint64_t kVolumeStride_ = 256;
		static constexpr uint64_t kVolumesOffset_ = kVolumeStride_ * kVolumeCount_;
		static constexpr uint64_t kFrameConstantStride_ = kVolumesOffset_ + 768;
		static_assert(sizeof(DDGIVolumeData) * kVolumeCount_ <= 768);

		struct AtlasTexture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
			uint32_t width = 0;
			uint32_t height = 0;
		};

		enum TimerID : uint32_t {
			kTimer_Trace = 0,
			kTimer_Update,
			kTimer_Count,
		};

		AtlasTexture CreateAtlas(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format);
		/// <summary>このフレームの格子の位置とレイの回転を決めて定数へ書く</summary>
		void UpdateVolume(uint32_t volumeIndex, uint32_t frameIndex);
		/// <summary>1つの格子のレイを飛ばし、再配置して畳み込む</summary>
		void RenderVolume(uint32_t volumeIndex, RaytracingScene& scene);
		D3D12_GPU_VIRTUAL_ADDRESS GetVolumeGPUVirtualAddress(uint32_t volumeIndex) const;

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		Settings settings_;

		AtlasTexture rayData_;
		AtlasTexture irradiance_;
		AtlasTexture distance_;
		// プローブごとの再配置のずれ
		AtlasTexture probeData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> volumeResource_;
		uint8_t* volumeMapped_ = nullptr;
		DDGIVolumeData volumes_[kVolumeCount_];

		// 次の更新で全プローブを作り直す
		bool needsReset_ = true;
		bool hasBaseCell_[kVolumeCount_] = {};
		uint32_t updateCounter_[kVolumeCount_] = {};
		std::mt19937 random_{ 12345u };

		DXC::GPUTimer gpuTimer_;
	};

}
