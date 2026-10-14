#pragma once
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"

namespace Graphics {
	class LightManager;
}

namespace GameObject {

	/// <summary>
	/// 配置データに置く点光源・スポットライト
	/// </summary>
	/// <remarks>位置と向きは Transform から取る。LightManager の枠を1つ使う</remarks>
	class PlacedLight : public GameObject {
	public:

		enum class Type {
			Point,
			Spot,
		};

		/// <summary>配置データ1件から生成する</summary>
		void Create(const nlohmann::json& data, Graphics::LightManager* lightManager);
		/// <summary>既定の設定で生成する。エディタからの追加用</summary>
		void Create(Type type, Graphics::LightManager* lightManager);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		/// <summary>設定と今の位置・向きを LightManager へ入れる</summary>
		void Update() override;
		void Draw(bool is = false) override;
		void ParameterGUI() override;

		/// <summary>消灯する。破棄する前に呼ぶこと</summary>
		/// <remarks>LightManager の枠は詰めないので、呼ばないと最後の明るさのまま光り続ける</remarks>
		void TurnOff();

	private:

		/// <summary>LightManager に枠を1つ取る</summary>
		void Register(Graphics::LightManager* lightManager);
		void Apply(float intensity);

	private:

		Graphics::LightManager* lightManager_ = nullptr;
		/// <summary>LightManager 側の番号。枠が埋まっていて取れなければ -1</summary>
		int slot_ = -1;

		Type type_ = Type::Point;
		Math::Vector3 color_ = { 1.0f,1.0f,1.0f };
		float intensity_ = 5.0f;
		/// <summary>光が届く距離</summary>
		float range_ = 10.0f;
		float decay_ = 1.0f;
		/// <summary>スポットライトの広がり。中心から端までの角度(度)</summary>
		float angle_ = 30.0f;
		/// <summary>スポットライトの端のぼけ。0で硬く、1で中心からなだらか</summary>
		float softness_ = 0.3f;
	};

}
