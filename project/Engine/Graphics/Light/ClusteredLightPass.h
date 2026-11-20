#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Matrix/Matrix4x4.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	struct ClusteredLightData;

	/// <summary>
	/// 影を落とさないライトのクラスタ化ライティング (Forward+)
	/// </summary>
	/// <remarks>クラスタごとに届くライトの一覧を CS で作る。ライトが無いフレームは走らない</remarks>
	class ClusteredLightPass {
	public:
		// Common/ClusteredLights.hlsli と同じ値であること
		static constexpr uint32_t kClusterCountX = 16;
		static constexpr uint32_t kClusterCountY = 9;
		static constexpr uint32_t kClusterCountZ = 24;
		static constexpr uint32_t kMaxLightsPerCluster = 128;
		static constexpr uint32_t kClusterCount = kClusterCountX * kClusterCountY * kClusterCountZ;

		ClusteredLightPass() = default;
		~ClusteredLightPass() = default;

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		/// <summary>このフレームのライトを上げ、クラスタごとの一覧を作る</summary>
		/// <remarks>前方描画より前に呼ぶ</remarks>
		void Render();

		/// <summary>このフレームに点いている影なしのライトの数</summary>
		uint32_t GetLightCount() const { return lightCount_; }
		/// <summary>このフレームに棒か長方形の光源があるか</summary>
		bool HasShapedLights() const { return hasShapedLights_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetParamsGPUVirtualAddress() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetLightsSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetCountsSrvHandle() const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetIndicesSrvHandle() const;

		void DebugGUI();

	private:

		/// <summary>HLSL 側 (Common/ClusteredLights.hlsli) の ClusterParams と並びが一致していること</summary>
		struct ClusterParams {
			Math::Matrix4x4 view;
			float projectionScale[2] = {};
			float clusterNear = 0.0f;
			float sliceScale = 0.0f;
			float screenSize[2] = {};
			uint32_t lightCount = 0;
			float clusterFar = 0.0f;
		};
		static_assert(sizeof(ClusterParams) == 96, "ClusterParams の大きさが ClusteredLights.hlsli と一致していない");

		// 段はこの距離の間を対数で区切る。いちばん奥の段は無限
		static constexpr float kClusterNear_ = 0.5f;
		static constexpr float kClusterFar_ = 300.0f;

		struct Buffer {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			uint32_t uavIndex = 0;
		};

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		uint32_t lightCount_ = 0;
		bool hasShapedLights_ = false;

		// ライトの一覧 (毎フレーム CPU から書く)
		Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_[DXC::kFrameCount_];
		ClusteredLightData* lightMapped_[DXC::kFrameCount_] = {};
		uint32_t lightSrvIndex_[DXC::kFrameCount_] = {};

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_[DXC::kFrameCount_];
		ClusterParams* paramMapped_[DXC::kFrameCount_] = {};

		// クラスタごとのライトの数と番号
		Buffer counts_;
		Buffer indices_;
	};

}
