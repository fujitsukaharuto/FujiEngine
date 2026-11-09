#pragma once
#include <optional>
#include <string>
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"
#include "Engine/GameObject/EmissiveFlicker.h"

namespace Graphics { class LightManager; }

namespace GameObject {

	/// <summary>
	/// 配置データから作られる、動きを持たないオブジェクト
	/// </summary>
	/// <remarks>地形や背景の置物用。見た目の設定もデータに持つ。生成・保存は Scene::Level が行う</remarks>
	class PlacedObject : public GameObject {
	public:

		/// <summary>配置データ1件から生成する</summary>
		void Create(const nlohmann::json& data);
		/// <summary>モデル名だけで生成する。エディタからの追加用</summary>
		void Create(const std::string& modelName);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Update() override;

		/// <summary>発光で周りを照らすときに点光源の枠を借りる先。Create より前に渡す</summary>
		void SetLightManager(Graphics::LightManager* lightManager) { lightManager_ = lightManager; }
		/// <summary>周りを照らす光を消す。配置から外す直前に呼ぶ</summary>
		/// <remarks>枠は返さない。LightManager が先に消えることがあるので、デストラクタでは触らない</remarks>
		void TurnOffLight();
		/// <summary>名前と見た目の設定。ここで変えた値が保存される</summary>
		void ParameterGUI() override;

	private:

		/// <summary>モデルを作り、見た目の設定を当てる</summary>
		void Build();
		/// <summary>データに書いてある見た目の設定だけをモデルへ当てる</summary>
		void ApplyLook();
		/// <summary>発光の色で周りを照らす点光源を、今の位置と設定に合わせる</summary>
		void ApplyEmissiveLight(bool isOn);
		/// <summary>前へ進め、決めた範囲の端まで来たら反対の端へ戻す</summary>
		void UpdateDrive();
		/// <summary>車のライトを今の位置へ</summary>
		void ApplyCarLights(bool isOn);
		/// <summary>発光の明るさを明滅の倍率に合わせる</summary>
		void ApplyFlicker();

	private:

		/// <summary>配置データが持つ見た目の設定</summary>
		/// <remarks>optional は、書いてある項目だけモデルの既定値を上書きするため</remarks>
		struct Look {
			std::string model;
			/// <summary>AnimationModel で作るか。映り込み(isMirror)とアニメーションはこちらでしか使えない</summary>
			bool isAnime = false;
			/// <summary>モデルファイルのアニメーションを再生する(isAnime のときだけ)</summary>
			bool isAnimation = false;
			/// <summary>空欄ならモデルのテクスチャのまま</summary>
			std::string texture;
			std::optional<Math::Vector4> color;
			std::optional<Math::Vector2> uvScale;
			std::optional<float> roughness;
			std::optional<float> metallic;
			std::optional<float> environment;
			/// <summary>自分で光る色。実際の明るさはこれに emissiveIntensity を掛けたもの</summary>
			std::optional<Math::Vector3> emissive;
			float emissiveIntensity = 1.0f;
			/// <summary>発光が周りを照らす距離。0なら照らさない。点光源の枠を1つ使う</summary>
			float emissiveLightRange = 0.0f;
			/// <summary>周りを照らす強さ。発光の明るさに掛ける</summary>
			float emissiveLightIntensity = 1.0f;
			/// <summary>リムライトの色。実際の明るさはこれに rimIntensity を掛けたもの</summary>
			std::optional<Math::Vector3> rim;
			float rimIntensity = 1.0f;
			float rimPower = 3.0f;
			bool isMirror = false;
			std::optional<Graphics::LightMode> light;
			/// <summary>モデルファイルのマテリアルをそのまま使う。既定はテクスチャと色だけ</summary>
			bool useModelMaterials = false;
		};
		Look look_;

		/// <summary>走らせる設定。前(+Z)へ進む</summary>
		struct Drive {
			float speed = 8.0f;
			/// <summary>進む向きの軸 (x か z) で、この範囲を出たら反対の端へ戻す</summary>
			float loopMin = -125.0f;
			float loopMax = 125.0f;
		};
		std::optional<Drive> drive_;

		/// <summary>車のライトの設定。位置はモデルの座標。左右は x を反転して置く</summary>
		struct CarLights {
			Math::Vector3 front = { 0.5f,0.65f,1.3f };
			Math::Vector3 rear = { 0.5f,0.65f,-1.35f };
			float headIntensity = 4.0f;
			float headRange = 12.0f;
			float tailIntensity = 0.8f;
			float tailRange = 3.0f;
		};
		std::optional<CarLights> carLights_;
		// 借りた影なしのライトの番号。-1 は借りていない
		int carLightSlots_[4] = { -1,-1,-1,-1 };

		EmissiveFlicker flicker_;
		/// <summary>作ってからの時間(秒)。明滅に使う</summary>
		float time_ = 0.0f;

		Graphics::LightManager* lightManager_ = nullptr;
		// 借りた点光源の番号。初めて照らすときに借りる。-1 は借りていない
		int lightSlot_ = -1;
		// モデルの原点からいちばん遠い頂点までの距離(拡縮前)
		float modelRadius_ = 0.0f;
	};

}
