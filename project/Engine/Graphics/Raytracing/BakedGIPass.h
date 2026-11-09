#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	class RaytracingScene;

	/// <summary>
	/// 焼く間接光。置いた箱の中のプローブをエディタで焼き、ファイルに残す
	/// </summary>
	/// <remarks>光は成分 (空の上・下・時刻ごとの太陽) に分けて焼き、引くときに今の色を掛ける</remarks>
	class BakedGIPass {
	public:
		/// <summary>レベルに置いた箱1つ</summary>
		struct VolumeDesc {
			Math::Vector3 center = { 0.0f,0.0f,0.0f };
			Math::Vector3 size = { 10.0f,4.0f,10.0f };
			/// <summary>プローブの間隔(m)</summary>
			float spacing = 1.0f;
		};

		/// <summary>太陽を焼く1つの時刻</summary>
		struct SunKey {
			float hour = 12.0f;
			/// <summary>地面から太陽へ向かう向き</summary>
			Math::Vector3 toSun = { 0.0f,1.0f,0.0f };
		};

		/// <summary>DDGICommon.hlsli の kDDGIBake* と同じ値であること</summary>
		enum BakeComponent : uint32_t {
			kBakeSkyUp = 1,
			kBakeSkyDown = 2,
			kBakeSun = 3,
		};

		/// <summary>置ける箱の数。BakedGI.hlsli の kBakedGIMaxVolumes と一致していること</summary>
		static constexpr uint32_t kMaxVolumes = 3;

		BakedGIPass() = default;
		~BakedGIPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>焼き始める。終わったら directory へ保存する</summary>
		/// <param name="sunKeys">太陽を焼く時刻。時刻で変えないなら1つ</param>
		/// <param name="hasSunHours">sunKeys の時刻に意味があるか。false ならいつでも1つ目を使う</param>
		void StartBake(const std::vector<VolumeDesc>& volumes, const std::vector<SunKey>& sunKeys, bool hasSunHours,
			const std::string& directory);
		void CancelBake();
		/// <summary>directory に保存した値を読む。読めなければ焼いた GI は無し</summary>
		bool Load(const std::string& directory);
		/// <summary>焼いた値を捨てる(ファイルは消さない)</summary>
		void Clear();

		bool IsBaking() const { return isBaking_; }
		/// <summary>焼いている途中の進み具合(0〜1)</summary>
		float GetBakeProgress() const;
		/// <summary>焼いた値を引ける状態か</summary>
		bool HasData() const { return hasData_; }
		/// <summary>焼いたときの箱。焼き直しが要るかを見る</summary>
		const std::vector<VolumeDesc>& GetBakedVolumes() const { return bakedVolumes_; }

		/// <summary>今の時刻。どの太陽の間を補間するかを決める</summary>
		/// <param name="hasHour">時刻で変える設定か。false なら焼いた1つ目の太陽を使う</param>
		void SetSunHour(bool hasHour, float hour) {
			hasSunHour_ = hasHour;
			sunHour_ = hour;
		}

		/// <summary>焼いている途中なら何回ぶんか進め、引く側の定数を入れる</summary>
		/// <remarks>TLAS を組んだ後、前方描画より前に呼ぶ</remarks>
		void Render(RaytracingScene& scene);

		void DebugGUI();

		//========================================================================*/
		//* Getter (焼いていなくても有効なものを返す)
		D3D12_GPU_VIRTUAL_ADDRESS GetParamsGPUVirtualAddress() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetSkySrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetSunSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetDistanceSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetProbeDataSrvHandle() const;

	private:

		/// <summary>BakedGI.hlsli の BakedGIParams と並びが一致していること</summary>
		struct BakedGIParams {
			DDGIVolumeData volumes[kMaxVolumes];
			uint32_t volumeCount = 0;
			uint32_t skyDownRow = 0;
			uint32_t sunRowA = 0;
			uint32_t sunRowB = 0;
			float sunRadiance[3] = {};
			float sunBlend = 0.0f;
			float sunAtlasSize[2] = {};
			float pad[2] = {};
		};
		static_assert(sizeof(BakedGIParams) == 576, "BakedGIParams の大きさが BakedGI.hlsli と一致していない");

		struct AtlasTexture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
			uint32_t width = 0;
			uint32_t height = 0;
		};

		/// <summary>箱からプローブの並びを決めたもの</summary>
		struct Layout {
			std::vector<DDGIVolumeData> volumes;
			uint32_t tileColumns = 0;
			uint32_t tileRows = 0;
			uint32_t probeCount = 0;
		};

		/// <summary>焼く仕事1つ。成分1つぶん</summary>
		struct Job {
			BakeComponent component = kBakeSkyUp;
			uint32_t sunKey = 0;
		};

		static constexpr uint32_t kIrradianceTexels_ = 8;
		static constexpr uint32_t kDistanceTexels_ = 16;
		static constexpr uint32_t kFixedRays_ = 32;
		static constexpr uint32_t kRayThreadGroup_ = 32;
		/// <summary>1回に1プローブから飛ばすレイ。DDGI の上限と同じ</summary>
		static constexpr uint32_t kBakeRays_ = 128;
		/// <summary>成分1つを焼く回数</summary>
		static constexpr uint32_t kBakeIterations_ = 64;
		/// <summary>最初の成分の前に、物に埋まったプローブをずらす回数</summary>
		static constexpr uint32_t kRelocationIterations_ = 16;
		/// <summary>1フレームに進める回数</summary>
		static constexpr uint32_t kIterationsPerFrame_ = 4;
		/// <summary>アトラスの横の上限 (16384 / 18 texel)</summary>
		static constexpr uint32_t kMaxTileColumns_ = 900;
		/// <summary>レイの結果の縦の上限 (テクスチャの高さ)</summary>
		static constexpr uint32_t kMaxProbes_ = 16384;
		static constexpr int32_t kMaxProbesPerAxis_ = 64;

		// 定数の並び: 回ごと[箱ごと(256) → 全部の箱(768)] × kIterationsPerFrame_ → 引く側(768)
		static constexpr uint64_t kVolumeStride_ = 256;
		static constexpr uint64_t kVolumesOffset_ = kVolumeStride_ * kMaxVolumes;
		static constexpr uint64_t kIterationStride_ = kVolumesOffset_ + 768;
		static constexpr uint64_t kParamsOffset_ = kIterationStride_ * kIterationsPerFrame_;
		static constexpr uint64_t kFrameConstantStride_ = kParamsOffset_ + 768;
		static_assert(sizeof(DDGIVolumeData) * kMaxVolumes <= 768);

		/// <summary>箱からプローブの並びを決める。並べきれないときは間隔を広げる</summary>
		static Layout MakeLayout(const std::vector<VolumeDesc>& volumes);
		/// <summary>並びに合わせてテクスチャを作り直す</summary>
		void CreateTextures(const Layout& layout, uint32_t sunKeyCount);
		AtlasTexture CreateAtlas(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState);
		void RetireAtlas(AtlasTexture& atlas);

		/// <summary>焼く1回ぶん。成分の仕事の iteration 回目</summary>
		void RunIteration(RaytracingScene& scene, uint32_t slot);
		/// <summary>引く側の定数を、今の時刻と太陽から決める</summary>
		void UpdateParams(uint32_t frameIndex);

		bool Save(const std::string& directory);

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		Layout layout_;
		std::vector<VolumeDesc> bakedVolumes_;
		std::vector<SunKey> sunKeys_;
		bool hasSunHours_ = false;
		bool hasData_ = false;

		AtlasTexture rayData_;
		AtlasTexture sky_;
		AtlasTexture sun_;
		AtlasTexture distance_;
		AtlasTexture probeData_;

		/// <summary>作り直す前のテクスチャ。この回数の Render の後に捨てる</summary>
		struct Retired {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t framesLeft = 0;
		};
		std::vector<Retired> retired_;

		Microsoft::WRL::ComPtr<ID3D12Resource> constantResource_;
		uint8_t* constantMapped_ = nullptr;

		// 焼いている途中の状態
		bool isBaking_ = false;
		std::vector<Job> jobs_;
		uint32_t jobIndex_ = 0;
		uint32_t iteration_ = 0;
		uint32_t totalIterations_ = 0;
		uint32_t doneIterations_ = 0;
		std::string bakeDirectory_;
		/// <summary>0 でなければ、この回数の Render の後に保存する</summary>
		uint32_t saveCountdown_ = 0;
		std::mt19937 random_{ 20261026u };

		bool hasSunHour_ = false;
		float sunHour_ = 12.0f;
	};

}
