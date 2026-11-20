#pragma once
#include <optional>
#include "Engine/Math/Spline/Spline.h"
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
		/// <summary>メッシュ単位で選んだときの見た目の GUI。変えた項目は配置データに残る</summary>
		void MeshInspectorGUI(int meshIndex) override;
		/// <summary>見た目のテクスチャを差し替える (保存される)</summary>
		void SetLookTexture(const std::string& texture);

		/// <summary>沿って進む曲線の名前。設定が無ければ nullptr</summary>
		const std::string* GetFollowSplineName() const { return follow_ ? &follow_->spline : nullptr; }
		/// <summary>曲線に沿って1フレームぶん進め、位置と向き (進む向きへ +Z) を入れる</summary>
		/// <remarks>Level が Update の前に呼ぶ。閉じた曲線は周回、開いた曲線は終わりから始めへ戻る</remarks>
		void FollowSpline(const Math::Spline& spline);

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
		/// <summary>雨のとき、4つのタイヤから水はねを出す</summary>
		void EmitWheelSpray();
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
			/// <summary>光る所と色のテクスチャ。空欄ならモデルのまま (modelMaterials でなければ無し)</summary>
			std::string emissiveTexture;
			std::optional<Math::Vector4> color;
			std::optional<Math::Vector2> uvScale;
			std::optional<float> roughness;
			std::optional<float> metallic;
			std::optional<float> environment;
			/// <summary>壁の汚れの強さ</summary>
			std::optional<float> weathering;
			/// <summary>流れる水の速さ (m/s)。u が小さくなる向きへ流れる</summary>
			std::optional<float> flowWater;
			/// <summary>クリアコートの強さ (0〜1)。粗さは clearCoatRoughness</summary>
			std::optional<float> clearCoat;
			float clearCoatRoughness = 0.05f;
			/// <summary>異方性 (-1〜1)。向きは anisotropyRotation (度) だけ接線から回す</summary>
			std::optional<float> anisotropy;
			float anisotropyRotation = 0.0f;
			/// <summary>布の光沢の色。粗さは sheenRoughness</summary>
			std::optional<Math::Vector3> sheen;
			float sheenRoughness = 0.5f;
			/// <summary>視差マッピングの高さのテクスチャ (R、白が高い)。空なら視差なし</summary>
			std::string heightTexture;
			/// <summary>高さ1の段差が UV で何ぶんずれて見えるか</summary>
			float heightScale = 0.05f;
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

		/// <summary>曲線に沿って進む設定</summary>
		struct SplineFollow {
			std::string spline;		// 曲線の名前 (Level::FindSpline)
			float speed = 8.0f;		// m/s。負なら逆向き
			float offset = 0.0f;	// 始める位置 (曲線の始点からの距離 m)
			float travelled = 0.0f;	// 進んだ距離。保存しない
		};
		std::optional<SplineFollow> follow_;

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

		/// <summary>
		/// 置いた物のあたり判定。大きさとずれは物の拡縮前の値で、物と一緒に拡縮する
		/// </summary>
		struct ColliderDesc {
			Collision::ColliderShape shape = Collision::ColliderShape::Box;
			Math::Vector3 size = { 1.0f,1.0f,1.0f };	// 箱
			float radius = 0.5f;						// 球・カプセル
			float height = 2.0f;						// カプセル (両端の半球を含む)
			int axis = 1;								// カプセルの向き 0:X 1:Y 2:Z
			Math::Vector3 offset{};
			uint32_t layer = 1u;
			uint32_t mask = Collision::kAllLayers;
			std::string tag;
		};
		std::optional<ColliderDesc> collider_;

		/// <summary>
		/// メッシュごとに上書きする見た目 (Inspector でメッシュを選んで変える)
		/// </summary>
		struct MeshLook {
			int index = 0;
			std::optional<Math::Vector4> color;
			std::optional<float> roughness;
			std::optional<float> metallic;
			std::optional<Math::Vector3> emissive;
		};
		std::vector<MeshLook> meshLooks_;
		/// <summary>meshLooks_ をマテリアルへ当てる。ApplyLook の最後に呼ぶ</summary>
		void ApplyMeshLooks();
		/// <summary>collider_ からコライダーを作り直す</summary>
		void ApplyCollider();
		/// <summary>今の拡縮でコライダーの大きさとずれを入れ直す</summary>
		void ApplyColliderSize();
		/// <summary>モデルの頂点を包む大きさにする</summary>
		void FitColliderToModel();
	};

}
