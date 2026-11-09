#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Vector/Vector3.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	class RaytracingScene;
	class ClusteredLightPass;

	/// <summary>
	/// 反射プローブ。置いた箱の中心から周りをキューブに焼き、箱の中の映り込みに使う
	/// </summary>
	/// <remarks>起動時と光が変わったときにレイトレで焼く。空は焼かず前方描画が今の空を足す</remarks>
	class ReflectionProbePass {
	public:
		/// <summary>レベルに置いた箱1つ</summary>
		struct ProbeDesc {
			Math::Vector3 center = { 0.0f,2.0f,0.0f };
			Math::Vector3 size = { 10.0f,4.0f,10.0f };
			/// <summary>箱の端からこの距離で外へ溶かす(m)</summary>
			float blendDistance = 1.0f;
			float intensity = 1.0f;
			/// <summary>映った物を箱の壁にあるとみなして向きを直すか</summary>
			bool boxProjection = true;
		};

		/// <summary>置ける箱の数。ReflectionProbe.hlsli の kReflectionProbeMaxCount と一致していること</summary>
		static constexpr uint32_t kMaxProbes = 8;

		ReflectionProbePass() = default;
		~ReflectionProbePass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>レベルの箱を渡す。前と違えば焼き直す</summary>
		void SetProbes(const std::vector<ProbeDesc>& probes);
		/// <summary>全部の箱を焼き直す</summary>
		void RequestRebake();

		/// <summary>このフレームに焼くか。焼くフレームは TLAS に物の見た目を集める</summary>
		bool NeedsRayLooks() const { return !bakeQueue_.empty(); }
		bool IsBaking() const { return !bakeQueue_.empty(); }
		uint32_t GetProbeCount() const { return static_cast<uint32_t>(probes_.size()); }
		/// <summary>一度でも焼けた箱の数</summary>
		uint32_t GetBakedCount() const;

		/// <summary>光が変わったら自動で焼き直すか</summary>
		void SetAutoRebake(bool enabled) { isAutoRebake_ = enabled; }
		bool IsAutoRebake() const { return isAutoRebake_; }

		/// <summary>焼く箱があれば1つ焼き、引く側の定数を入れる</summary>
		/// <remarks>TLAS を組んで影なしのライトを集めた後、前方描画より前に呼ぶ</remarks>
		/// <param name="hasRayLooks">TLAS に物の見た目を集めたか。集めていなければ焼かない</param>
		void Render(RaytracingScene& scene, const ClusteredLightPass& clusteredLights, bool hasRayLooks);

		void DebugGUI();

		//========================================================================*/
		//* Getter (焼いていなくても有効なものを返す)
		D3D12_GPU_VIRTUAL_ADDRESS GetParamsGPUVirtualAddress() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetProbesSrvHandle() const;

	private:

		/// <summary>ReflectionProbe.hlsli の ReflectionProbe と並びが一致していること</summary>
		struct ProbeData {
			float capturePosition[3] = {};
			float blendDistance = 1.0f;
			float boxMin[3] = {};
			float intensity = 1.0f;
			float boxMax[3] = {};
			uint32_t flags = 0;
		};
		static_assert(sizeof(ProbeData) == 48, "ProbeData の大きさが ReflectionProbe.hlsli と一致していない");

		/// <summary>ReflectionProbe.hlsli の ReflectionProbeParams と並びが一致していること</summary>
		struct ProbeParams {
			ProbeData probes[kMaxProbes];
			uint32_t probeCount = 0;
			float pad[3] = {};
		};
		static_assert(sizeof(ProbeParams) == 400, "ProbeParams の大きさが ReflectionProbe.hlsli と一致していない");

		/// <summary>ReflectionProbeCapture.CS.hlsl の ReflectionProbeCaptureParam と並びが一致していること</summary>
		struct CaptureParam {
			float capturePosition[3] = {};
			uint32_t clusteredLightCount = 0;
			float maxDistance = 1000.0f;
			uint32_t captureSize = 0;
			float textureLod = 0.0f;
			float pad = 0.0f;
		};

		/// <summary>ReflectionProbePrefilter.CS.hlsl の ProbePrefilterParam と並びが一致していること</summary>
		struct PrefilterParam {
			float roughness = 0.0f;
			uint32_t sampleCount = 0;
			uint32_t sourceSize = 0;
			uint32_t sliceOffset = 0;
			uint32_t outputSize = 0;
			uint32_t pad[3] = {};
		};

		/// <summary>焼くときの光。変わったら焼き直す</summary>
		struct LightingKey {
			float values[16] = {};
			uint32_t clusteredLightCount = 0;
			int32_t punctualCount = 0;
		};

		// 映り込みの解像度
		static constexpr uint32_t kCaptureSize_ = 128;
		// ミップを1x1まで作る
		static constexpr uint32_t kCaptureMipLevels_ = 8;
		// ミップ番号が粗さ。IBL.hlsli の kPrefilteredMaxMip + 1 と一致していること
		static constexpr uint32_t kPrefilteredMipLevels_ = 5;
		static constexpr uint32_t kPrefilterSampleCount_ = 128;
		static constexpr uint32_t kCubeFaceCount_ = 6;
		static constexpr uint32_t kThreadGroupSize_ = 8;
		// 当たった物のテクスチャを引くミップ
		static constexpr float kCaptureTextureLod_ = 1.0f;
		static constexpr float kCaptureMaxDistance_ = 1000.0f;
		// 光が変わり続けている間に焼き直す間隔
		static constexpr uint32_t kAutoRebakeInterval_ = 30;

		// 定数の並び: 引く側(512) → 焼く(256) → 畳む(256 × ミップ)。フレームごとに1組
		static constexpr uint64_t kParamsOffset_ = 0;
		static constexpr uint64_t kCaptureOffset_ = 512;
		static constexpr uint64_t kPrefilterOffset_ = kCaptureOffset_ + 256;
		static constexpr uint64_t kFrameConstantStride_ = kPrefilterOffset_ + 256 * kPrefilteredMipLevels_;
		static_assert(sizeof(ProbeParams) <= 512);

		void CreateResources();
		/// <summary>slot 番の箱を焼いて畳む</summary>
		void BakeProbe(RaytracingScene& scene, const ClusteredLightPass& clusteredLights, uint32_t slot, uint32_t frameIndex);
		void UpdateParams(uint32_t frameIndex);
		LightingKey MakeLightingKey(uint32_t clusteredLightCount) const;
		static bool IsSameLighting(const LightingKey& a, const LightingKey& b);
		static bool IsSameProbe(const ProbeDesc& a, const ProbeDesc& b);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		std::vector<ProbeDesc> probes_;
		/// <summary>箱ごとに一度でも焼けたか</summary>
		bool isBaked_[kMaxProbes] = {};
		/// <summary>これから焼く箱の番号</summary>
		std::vector<uint32_t> bakeQueue_;

		bool isAutoRebake_ = true;
		bool hasLightingKey_ = false;
		LightingKey bakedLighting_;
		uint32_t framesSinceRebake_ = 0;

		// 焼いた直後の、畳む前のキューブ
		Microsoft::WRL::ComPtr<ID3D12Resource> capture_;
		uint32_t captureSrvIndex_ = 0;
		uint32_t captureUavIndex_[kCaptureMipLevels_] = {};

		// 全部の箱を畳んだキューブの配列
		Microsoft::WRL::ComPtr<ID3D12Resource> probeArray_;
		uint32_t probesSrvIndex_ = 0;
		uint32_t probesUavIndex_[kPrefilteredMipLevels_] = {};

		Microsoft::WRL::ComPtr<ID3D12Resource> constantResource_;
		uint8_t* constantMapped_ = nullptr;

		// 焼き直した回数
		uint32_t bakeCount_ = 0;
	};

}
