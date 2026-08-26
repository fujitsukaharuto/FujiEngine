#pragma once
#include <memory>
#include <vector>
// json は宣言とポインタにしか使わないので前方宣言で足りる
#include <json_fwd.hpp>

#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Object/RenderObject.h"
#include "Engine/Collision/AABBCollider.h"
// FPSKeeper / CameraManager / Input はここでは使わない。要る派生クラスが自分で include する

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
		/// <summary>基底が持つコンポーネント(コライダー等)を更新する</summary>
		/// <remarks>
		/// 派生は override して自分の処理を書き、**最後に GameObject::GameObject::Update() を呼ぶ**こと。
		/// 位置を動かした後に呼ばないと、当たり判定が1フレーム古い位置で行われる
		/// </remarks>
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

		/// <summary>値比較</summary>
		float ComparNum(float a, float b);
		/// <summary>モデル作成</summary>
		void CreateModel(const std::string& name);
		/// <summary>アニメーションモデル作成</summary>
		void CreateAnimeModel(const std::string& name);
		/// <summary>Jsonから作成</summary>
		void CreateFromJson(const std::string& name);
		void CreateFromJson();
		/// <summary>jsonからこのオブジェクトのTransformを読み込む</summary>
		void LoadTransformFromJson(const std::string& name);

		//========================================================================*/
		//* Setter
		void SetModel(const std::string& name);
		void SetAnimeModel(const std::string& name);
		void SetModelDataJson(const nlohmann::json& jsonData);

		//========================================================================*/
		//* Getter
		/// <remarks>生成していない側は nullptr を返す</remarks>
		Graphics::Object3d* GetModel() { return model_; }
		Graphics::AnimationModel* GetAnimeModel() { return animeModel_; }

		/// <summary>生成済みの描画オブジェクトを基底(RenderObject)として返す</summary>
		/// <remarks>model_ と animeModel_ は同時に生成しない設計。生成済みの方を返し、未生成なら nullptr</remarks>
		Graphics::RenderObject* GetRenderObject() {
			if (model_) { return model_; }
			return animeModel_;
		}
		const Graphics::RenderObject* GetRenderObject() const {
			if (model_) { return model_; }
			return animeModel_;
		}

		/// <summary>このオブジェクト自身の位置・回転・拡縮</summary>
		/// <remarks>描画モデルを持たなくても使える。モデルやコライダーはこれにぶら下がる</remarks>
		Math::Trans& GetTrans() { return transform_; }
		const Math::Trans& GetTrans() const { return transform_; }
		/// <summary>ペアレントを含めたワールド座標</summary>
		Math::Vector3 GetWorldPos()const { return transform_.GetWorldPos(); }

		/// <summary>コライダーの取得</summary>
		/// <remarks>1つしか持たないオブジェクト向け。複数持つ場合は GetColliders() を使う</remarks>
		Collision::BaseCollider* GetCollider() { return colliders_.empty() ? nullptr : colliders_.front().get(); }
		const std::vector<std::unique_ptr<Collision::AABBCollider>>& GetColliders() const { return colliders_; }

	protected:

		/// <summary>登録済みコライダーのワールド情報を更新する</summary>
		/// <remarks>通常は Update() が呼ぶので、派生が直接呼ぶ必要はない</remarks>
		void UpdateColliders();
		/// <summary>登録済みコライダーの判定ボリュームを描く(_DEBUGMODE のみ)</summary>
		void DrawColliders();

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

		/// <summary>描画物を renderers_ へ登録する</summary>
		/// <remarks>主ビジュアルは最後尾、子ビジュアルはその手前に入れて描画順を保つ</remarks>
		void RegisterRenderer(std::unique_ptr<Graphics::RenderObject> object, bool isPrimary);

		/// <summary>描画物の表示/非表示を切り替える</summary>
		/// <remarks>主ビジュアル(GetModel/GetAnimeModel)にも使える。未登録のハンドルは無視される</remarks>
		void SetRendererVisible(const Graphics::RenderObject* handle, bool visible);

	protected:

		/// <summary>このオブジェクト自身のTransform。描画物・コライダー・アンカーの親になる</summary>
		/// <remarks>描画モデルの中にあった位置情報をここへ引き上げたもの。モデルが無くても位置を持てる</remarks>
		Math::Trans transform_;

		/// <summary>基底が所有・描画するビジュアル1件</summary>
		struct RendererEntry {
			std::unique_ptr<Graphics::RenderObject> object;
			bool visible = true;
		};

		/// <summary>このオブジェクトが持つ描画物。主ビジュアルも子ビジュアルもここが所有する(=MeshRenderer相当)</summary>
		/// <remarks>
		/// **主ビジュアル(CreateModel系で作るもの)は必ず最後尾**。影のような非additiveの子ビジュアルは
		/// 本体より前に描く必要があるので、この並び順そのものが描画順の意味を持つ。
		/// 追加は RegisterRenderer() を通すこと(この不変条件をそこで守っている)
		/// </remarks>
		std::vector<RendererEntry> renderers_;

		/// <summary>主ビジュアルへのハンドル。実体の所有は renderers_ が持つ</summary>
		/// <remarks>Object3d と AnimationModel は同時に生成しない設計。生成していない側は nullptr</remarks>
		Graphics::Object3d* model_ = nullptr;
		Graphics::AnimationModel* animeModel_ = nullptr;

		/// <summary>このオブジェクトが持つあたり判定(=Colliderコンポーネント相当)</summary>
		std::vector<std::unique_ptr<Collision::AABBCollider>> colliders_;

		/// <summary>描画しないTransformアンカー。エミッタ・コライダーのペアレント先にだけ使う</summary>
		std::vector<std::unique_ptr<Math::Trans>> anchors_;

		/// <summary>Jsonから生成する際の元データ。json.hpp をヘッダから隔離するため実体は持たない</summary>
		std::unique_ptr<nlohmann::json> modelDataJson_;

	private:

	};

}
