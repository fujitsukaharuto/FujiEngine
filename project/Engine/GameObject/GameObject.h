#pragma once
#include <memory>
#include <vector>
#include <cassert>
// json は宣言とポインタにしか使わないので前方宣言で足りる
#include <json_fwd.hpp>

#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Object/RenderObject.h"
#include "Engine/Collision/AABBCollider.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Input/Input.h"

namespace GameObject {

	/// <summary>
	/// ゲームオブジェクトの基底クラス
	/// </summary>
	class GameObject {
	public:
		GameObject();
		/// <remarks>modelDataJson_ が不完全型の unique_ptr なので定義は .cpp に置く</remarks>
		virtual ~GameObject();

		virtual void Initialize();
		virtual void Update();
		virtual void Draw(bool is = false);
		virtual void DebugGUI();

	#ifdef _DEBUGMODE
		virtual void Debug();
	#endif // _DEBUG

	public:

		//========================================================================*/
		//* Collision
		// AddCollider で自動的に結線される。反応が要る派生クラスだけが override すればよい
		virtual void OnCollisionEnter([[maybe_unused]] const Collision::ColliderInfo& other) {}
		virtual void OnCollisionStay([[maybe_unused]] const Collision::ColliderInfo& other) {}
		virtual void OnCollisionExit([[maybe_unused]] const Collision::ColliderInfo& other) {}

		/// <summary>登録済みコライダーのワールド情報を更新する</summary>
		void UpdateColliders();
		/// <summary>登録済みコライダーの判定ボリュームを描く</summary>
		void DrawColliders();

		/// <summary>値比較</summary>
		float ComparNum(float a, float b);
		/// <summary>モデル作成</summary>
		void CreateModel(const std::string& name);
		/// <summary>アニメーションモデル作成</summary>
		void CreateAnimeModel(const std::string& name);
		/// <summary>Jsonから作成</summary>
		void CreateFromJson(const std::string& name);
		void CreateFromJson();

		//========================================================================*/
		//* Setter
		void SetModel(const std::string& name);
		void SetAnimeModel(const std::string& name);
		void SetModelDataJson(const nlohmann::json& jsonData);

		//========================================================================*/
		//* Getter
		/// <remarks>生成していない側は nullptr を返す</remarks>
		Graphics::Object3d* GetModel() { return model_.get(); }
		Graphics::AnimationModel* GetAnimeModel() { return animeModel_.get(); }

		/// <summary>生成済みの描画オブジェクトを基底(RenderObject)として返す</summary>
		/// <remarks>model_ と animeModel_ は同時に生成しない設計。生成済みの方を返し、未生成なら nullptr</remarks>
		Graphics::RenderObject* GetRenderObject() {
			if (model_) { return model_.get(); }
			return animeModel_.get();
		}
		const Graphics::RenderObject* GetRenderObject() const {
			if (model_) { return model_.get(); }
			return animeModel_.get();
		}

		// Object3d / AnimationModel のどちらで生成していても Transform・ワールド座標を取れる
		Math::Trans& GetTrans() { Graphics::RenderObject* o = GetRenderObject(); assert(o && "このオブジェクトは描画モデルを生成していない"); return o->GetTransform(); }
		Math::Vector3 GetWorldPos()const { const Graphics::RenderObject* o = GetRenderObject(); assert(o && "このオブジェクトは描画モデルを生成していない"); return o->GetWorldPos(); }

		/// <summary>コライダーの取得</summary>
		/// <remarks>1つしか持たないオブジェクト向け。複数持つ場合は GetColliders() を使う</remarks>
		Collision::BaseCollider* GetCollider() { return colliders_.empty() ? nullptr : colliders_.front().get(); }
		const std::vector<std::unique_ptr<Collision::AABBCollider>>& GetColliders() const { return colliders_; }

	protected:

		/// <summary>model_ を必要になった時点で生成する</summary>
		/// <remarks>model_ を直接操作したい派生クラスは、先にこれを通すこと</remarks>
		Graphics::Object3d* EnsureModel();
		/// <summary>animeModel_ を必要になった時点で生成する</summary>
		Graphics::AnimationModel* EnsureAnimeModel();

		/// <summary>子ビジュアル(Object3d)を空で生成・登録し、設定用ハンドルを返す</summary>
		/// <remarks>Create/CreateRing 等の生成は呼び出し側で行う。所有権は renderers_ が持つ</remarks>
		Graphics::Object3d* AddRenderer();
		/// <summary>子ビジュアル(Object3d)を name で生成(Create済)・登録し、ハンドルを返す</summary>
		Graphics::Object3d* AddRenderer(const std::string& name);
		/// <summary>Transformアンカー(エミッタやコライダーの親にするだけの点)を生成・登録し、ハンドルを返す</summary>
		/// <remarks>
		/// 描画しないので Object3d ではなく Math::Trans を使う(定数バッファもマテリアルも確保しない)。
		/// scale は 1 で初期化する。0 のままだと子の GetWorldMat が潰れ、RemoveScale では 0 除算になる。
		/// 所有権は anchors_ が持ち、ハンドルのアドレスは追加しても動かない
		/// </remarks>
		Math::Trans* AddAnchor();
		/// <summary>コライダーを生成・登録し、設定用ハンドルを返す</summary>
		/// <remarks>
		/// タグ・オーナー・3つのコールバックはここで結線するので、派生側はサイズと親だけ設定すればよい。
		/// 所有権は colliders_ が持つ
		/// </remarks>
		Collision::AABBCollider* AddCollider(const std::string& tag = "");

		/// <summary>子ビジュアルの表示/非表示を切り替える</summary>
		/// <remarks>条件付きで描いていたビジュアルを renderers_ に載せるためのもの。未登録のハンドルは無視される</remarks>
		void SetRendererVisible(const Graphics::RenderObject* handle, bool visible);

	protected:

		/// <summary>基底が所有・描画する子ビジュアル1件</summary>
		struct RendererEntry {
			std::unique_ptr<Graphics::RenderObject> object;
			bool visible = true;
		};

		std::unique_ptr<Graphics::Object3d> model_;
		std::unique_ptr<Graphics::AnimationModel> animeModel_;

		/// <summary>このオブジェクトにぶら下がる追加ビジュアル。生成/描画を一元管理する(=MeshRenderer相当)</summary>
		std::vector<RendererEntry> renderers_;

		/// <summary>このオブジェクトが持つあたり判定(=Colliderコンポーネント相当)</summary>
		std::vector<std::unique_ptr<Collision::AABBCollider>> colliders_;

		/// <summary>描画しないTransformアンカー。エミッタ・コライダーのペアレント先にだけ使う</summary>
		std::vector<std::unique_ptr<Math::Trans>> anchors_;

		/// <summary>Jsonから生成する際の元データ。json.hpp をヘッダから隔離するため実体は持たない</summary>
		std::unique_ptr<nlohmann::json> modelDataJson_;

	private:

	};

}
