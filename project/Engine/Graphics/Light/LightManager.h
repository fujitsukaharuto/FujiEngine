#pragma once
#include "Engine/Graphics/Light/DirectionLight.h"
#include "Engine/Graphics/Light/PointLight.h"
#include "Engine/Graphics/Light/SpotLight.h"
#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl.h>

namespace DXC { class DXCom; }

namespace Graphics {

	const int kMaxDirectionalLights = 3;
	const int kMaxPointLights = 10;
	const int kMaxSpotLights = 10;

	/// <summary>
	/// レイトレ影を光源の種類ごとに切るビット
	/// </summary>
	/// <remarks>HLSL 側 (Common/RayTracedShadow.hlsli) の kShadowMask* と同じ値であること</remarks>
	enum RayTracedShadowMask : uint32_t {
		kShadowMaskDirectional = 1u << 0,
		kShadowMaskPoint = 1u << 1,
		kShadowMaskSpot = 1u << 2,
		kShadowMaskAll = kShadowMaskDirectional | kShadowMaskPoint | kShadowMaskSpot,
	};

	/// <summary>
	/// 全てのライトをまとめた構造体
	/// </summary>
	/// <remarks>
	/// HLSL 側の AllLights 構造体と並び順・パディングが完全に一致していること。
	/// float3 は float4 境界を跨げないので、後ろに詰める値は明示的に pad を置いて位置を固定する
	/// </remarks>
	struct AllLightsData {
		DirectionalLight directionalLights[kMaxDirectionalLights];
		PointLightData pointLights[kMaxPointLights];
		SpotLightData spotLights[kMaxSpotLights];
		int32_t numDirectionalLights;
		int32_t numPointLights;
		int32_t numSpotLights;
		float pad0;

		// 半球アンビエント。法線の上下で空色と地面色を混ぜる。
		// PBR にすると Half-Lambert の回り込みが無くなるので、これが無いと影側が真っ黒になる
		Math::Vector3 ambientSkyColor = { 0.35f, 0.42f, 0.55f };
		float ambientIntensity = 0.30f;
		Math::Vector3 ambientGroundColor = { 0.20f, 0.17f, 0.14f };

		uint32_t rayTracedShadowMask = kShadowMaskAll;

		// レイトレAO。半球アンビエントにだけ掛かる。
		// デノイザが無く粒状のノイズが出るので既定は無効
		float aoRadius = 2.0f;
		float aoIntensity = 1.0f;
		uint32_t aoSampleCount = 4;
		uint32_t enableAO = 0;

		// 鏡面の環境光の遮蔽。有効にすると遮られた場所が暗くなり既存の絵が変わるので既定は無効
		float reflectionMaxDistance = 100.0f;
		float reflectionIntensity = 1.0f;
		uint32_t enableReflection = 0;
		uint32_t pad2 = 0;
	};

	/// <summary>
	/// ライト管理クラス
	/// </summary>
	class LightManager {
	public:
		LightManager() = default;
		~LightManager() = default;

	public:

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();
		void Update();

		/// <summary>
		/// ライトの作成（初期化時にデフォルトを作成）
		/// </summary>
		void CreateLight();

		/// <summary>
		/// ポイントライトの追加
		/// </summary>
		void AddPointLight();
		void AddPointLight(const PointLightData& data);

		/// <summary>
		/// スポットライトの追加
		/// </summary>
		void AddSpotLight();
		void AddSpotLight(const SpotLightData& data);

		/// <summary>
		/// データをコマンドリストに送る
		/// </summary>
		void SetLightCommand(ID3D12GraphicsCommandList* commandList);

		//========================================================================*/
		//* Getter
		DirectionalLight* GetDirectionLight(int num = 0) { return &allLightsData_.directionalLights[num]; }
		PointLightData* GetPointLight(int num = 0) { return &allLightsData_.pointLights[num]; }
		SpotLightData* GetSpotLight(int num = 0) { return &allLightsData_.spotLights[num]; }

		int GetNumDirectionalLights() const { return allLightsData_.numDirectionalLights; }
		int GetNumPointLights() const { return allLightsData_.numPointLights; }
		int GetNumSpotLights() const { return allLightsData_.numSpotLights; }

		//========================================================================*/
		//* Setter
		void SetNumDirectionalLights(int num) { allLightsData_.numDirectionalLights = num; }
		void SetNumPointLights(int num) { allLightsData_.numPointLights = num; }
		void SetNumSpotLights(int num) { allLightsData_.numSpotLights = num; }

		void DebugGUI();

	private:
		void CopyData(uint32_t frameIndex = 0);

	private:

		DXC::DXCom* dxcommon_;
		
		Microsoft::WRL::ComPtr<ID3D12Resource> allLightsResource_[DXC::kFrameCount_];
		AllLightsData* allLightsDataGPU_[DXC::kFrameCount_];
		AllLightsData allLightsData_;

	};
}