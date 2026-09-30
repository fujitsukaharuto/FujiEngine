#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <json_fwd.hpp>

#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/Object/RenderObject.h"
#include "Engine/Collision/AABBCollider.h"

namespace GameObject {

	/// <summary>
	/// ゲームオブジェクトの基底クラス
	/// </summary>
	class GameObject {
	public:
		GameObject();
		virtual ~GameObject();

		virtual void Initialize();
		/// <summary>基底が持つコンポーネント(コライダー等)を更新する</summary>
		/// <remarks>派生は override 後に必ず GameObject::GameObject::Update() を呼ぶこと</remarks>
		virtual void Update();
		virtual void Draw(bool is = false);

		/// <summary>このオブジェクト固有の調整項目。Inspector の末尾に出る</summary>
		/// <remarks>Transform・描画物・コライダーは Inspector が出すので、ここには派生だけが持つ値を書く</remarks>
		virtual void ParameterGUI() {}
		/// <summary>Inspector に出す中身(描画物・コライダー・ParameterGUI)</summary>
		/// <remarks>Transform は Inspector 側がギズモと一緒に出すのでここでは扱わない</remarks>
		void InspectorGUI();

		//========================================================================*/
		//* Hierarchy
		/// <summary>生存中の全オブジェクト。生成順に並ぶ</summary>
		/// <remarks>コンストラクタで登録・デストラクタで解除されるので、呼び出し側の登録は要らない</remarks>
		static const std::vector<GameObject*>& GetAll();
		/// <summary>Hierarchy に出す名前。未設定ならクラス名を返す</summary>
		std::string GetName() const;
		/// <summary>名前空間を除いたクラス名</summary>
		std::string GetTypeName() const;
		void SetName(const std::string& name) { name_ = name; }
		/// <summary>t がこのオブジェクトの持つTransform(本体・描画物・アンカー)か</summary>
		/// <remarks>Hierarchy が親ポインタから「どのオブジェクトの子か」を引き当てるのに使う</remarks>
		bool OwnsTrans(const Math::Trans* t) const;
		/// <summary>ピッキングの番号がこのオブジェクトの描画物のものか</summary>
		bool OwnsObjID(int objID) const;

		//========================================================================*/
		//* 寿命
		/// <summary>動かす/止める</summary>
		/// <remarks>止めている間は当たり判定から外れ、Spawn したものなら Update・Draw も呼ばれない。
		/// プールした弾の待機中など「消さずに休ませる」ときに使う。自分で持っているオブジェクトの Update・Draw は持ち主が呼び分けること</remarks>
		void SetActive(bool active) { isActive_ = active; }
		bool IsActive() const { return isActive_; }
		/// <summary>破棄を予約する</summary>
		/// <remarks>Spawn したものは、そのフレームの当たり判定が済んだ後にシーンが破棄する。
		/// 予約した時点で当たり判定からは外れる。自分で持っているオブジェクトには印が付くだけ</remarks>
		void Destroy() { isDestroyed_ = true; }
		bool IsDestroyed() const { return isDestroyed_; }

	public:

		//========================================================================*/
		//* Collision
		// AddCollider で自動的に結線される。反応が要る派生クラスだけが override すればよい
		virtual void OnCollisionEnter([[maybe_unused]] const Collision::ColliderInfo& other) {}
		virtual void OnCollisionStay([[maybe_unused]] const Collision::ColliderInfo& other) {}
		virtual void OnCollisionExit([[maybe_unused]] const Collision::ColliderInfo& other) {}
		/// <summary>このオブジェクトのコライダーを判定に入れるか</summary>
		/// <remarks>AddCollider したコライダーは SceneManager が毎フレーム自動で集めるので、シーン側の登録は要らない。
		/// SetActive(false) の間は呼ばれずに外れる。動いているが当てたくない間(溜め中など)だけ false を返す。
		/// コライダー1個ずつ切るなら BaseCollider::SetIsCollisonCheck</remarks>
		virtual bool IsCollisionActive() const { return true; }

		/// <summary>値比較</summary>
		float ComparNum(float a, float b);
		/// <summary>モデル作成</summary>
		void CreateModel(const std::string& name);
		/// <summary>アニメーションモデル作成</summary>
		void CreateAnimeModel(const std::string& name);
		/// <summary>jsonからこのオブジェクトのTransformを読み込む</summary>
		void LoadTransformFromJson(const std::string& name);

		//========================================================================*/
		//* Setter
		void SetModel(const std::string& name);
		void SetAnimeModel(const std::string& name);

		//========================================================================*/
		//* Getter
		/// <remarks>生成していない側は nullptr を返す</remarks>
		Graphics::Object3d* GetModel() { return model_; }
		Graphics::AnimationModel* GetAnimeModel() { return animeModel_; }

		/// <summary>生成済みの描画オブジェクトを基底(RenderObject)として返す</summary>
		/// <remarks>生成済みの方を返し、未生成なら nullptr</remarks>
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
		/// <remarks>描画しないので Math::Trans を使う。scale は 1 で初期化する</remarks>
		Math::Trans* AddAnchor();
		/// <summary>コライダーを生成・登録し、設定用ハンドルを返す</summary>
		/// <remarks>タグ・オーナー・コールバックはここで結線するので、派生はサイズと親だけ設定すればよい</remarks>
		Collision::AABBCollider* AddCollider(const std::string& tag = "");

		/// <summary>描画物を renderers_ へ登録する</summary>
		/// <remarks>主ビジュアルは最後尾、子ビジュアルはその手前に入れて描画順を保つ</remarks>
		void RegisterRenderer(std::unique_ptr<Graphics::RenderObject> object, bool isPrimary);

		/// <summary>描画物の表示/非表示を切り替える</summary>
		/// <remarks>主ビジュアル(GetModel/GetAnimeModel)にも使える。未登録のハンドルは無視される</remarks>
		void SetRendererVisible(const Graphics::RenderObject* handle, bool visible);

		//========================================================================*/
		//* Parameter
		/// <summary>調整値を登録する。Inspector の Parameters に並び、保存した値は次の生成から読み込まれる</summary>
		/// <remarks>既定値を入れた後に呼ぶこと。保存済みの値があればその場で上書きする。
		/// 保存先はクラスごとに1ファイル(resource/Json/Param/クラス名.json)なので、同じクラスのインスタンスは値を共有する</remarks>
		void AddParam(const std::string& name, float& value, float speed = 0.01f);
		void AddParam(const std::string& name, int& value);
		void AddParam(const std::string& name, bool& value);
		void AddParam(const std::string& name, Math::Vector3& value, float speed = 0.01f);
		/// <remarks>色として編集する</remarks>
		void AddParam(const std::string& name, Math::Vector4& value);

	protected:

		/// <summary>このオブジェクト自身のTransform。描画物・コライダー・アンカーの親になる</summary>
		/// <remarks>描画モデルの中にあった位置情報をここへ引き上げたもの。モデルが無くても位置を持てる</remarks>
		Math::Trans transform_;

		/// <summary>基底が所有・描画するビジュアル1件</summary>
		struct RendererEntry {
			std::unique_ptr<Graphics::RenderObject> object;
			bool visible = true;
		};

		/// <summary>このオブジェクトが持つ描画物。主ビジュアルも子ビジュアルもここが所有する</summary>
		/// <remarks>主ビジュアルは最後尾。並び順がそのまま描画順になる</remarks>
		std::vector<RendererEntry> renderers_;

		/// <summary>主ビジュアルへのハンドル。実体の所有は renderers_ が持つ</summary>
		/// <remarks>生成していない側は nullptr</remarks>
		Graphics::Object3d* model_ = nullptr;
		Graphics::AnimationModel* animeModel_ = nullptr;

		/// <summary>このオブジェクトが持つあたり判定(=Colliderコンポーネント相当)</summary>
		std::vector<std::unique_ptr<Collision::AABBCollider>> colliders_;

		/// <summary>描画しないTransformアンカー。エミッタ・コライダーのペアレント先にだけ使う</summary>
		std::vector<std::unique_ptr<Math::Trans>> anchors_;

	private:

		/// <summary>AddParam で登録した調整値1件。値の実体は派生のメンバで、ここはその場所を覚えるだけ</summary>
		struct Param {
			std::string name;
			std::variant<float*, int*, bool*, Math::Vector3*, Math::Vector4*> value;
			float speed = 0.01f;
		};

		/// <summary>保存済みの値があれば読み込んでから登録する</summary>
		void RegisterParam(Param param);
		/// <summary>登録済みの調整値をクラスのファイルへ書き出す</summary>
		void SaveParams() const;

	private:

		/// <summary>Hierarchy に出す名前。空ならクラス名で代用する</summary>
		std::string name_;
		std::vector<Param> params_;
		bool isActive_ = true;
		bool isDestroyed_ = false;

	};

}
