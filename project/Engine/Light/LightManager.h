#pragma once
#include "DirectionLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include <d3d12.h>
#include <memory>

class DXCom;

namespace Graphics {
	/// <summary>
	/// ライト管理クラス
	/// </summary>
	class LightManager {
	public:
		LightManager() = default;
		~LightManager() = default;

	public:

		void Initialize(DXCom* pDxcom);
		void Finalize();
		void Update();

		/// <summary>
		/// ライトの作成
		/// </summary>
		void CreateLight();

		/// <summary>
		/// ポイントライトの追加
		/// </summary>
		void AddPointLight();

		/// <summary>
		/// スポットライトの追加
		/// </summary>
		void AddSpotLight();

		/// <summary>
		/// データをコマンドリストに送る
		/// </summary>
		void SetLightCommand(ID3D12GraphicsCommandList* commandList);

		//========================================================================*/
		//* Getter
		DirectionLight* GetDirectionLight() { return directionLight_.get(); }
		PointLight* GetPointLight(int num = 0) { return pointLights_[num].get(); }
		SpotLight* GetSpotLight(int num = 0) { return spotLights_[num].get(); }

		void DebugGUI();

	private:

	private:

		DXCom* dxcommon_;
		std::unique_ptr<DirectionLight> directionLight_;
		std::vector <std::unique_ptr<PointLight>> pointLights_;
		std::vector<std::unique_ptr<SpotLight>> spotLights_;

	};
}