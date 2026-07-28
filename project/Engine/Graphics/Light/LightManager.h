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
	/// 全てのライトをまとめた構造体
	/// </summary>
	struct AllLightsData {
		DirectionalLight directionalLights[kMaxDirectionalLights];
		PointLightData pointLights[kMaxPointLights];
		SpotLightData spotLights[kMaxSpotLights];
		int32_t numDirectionalLights;
		int32_t numPointLights;
		int32_t numSpotLights;
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