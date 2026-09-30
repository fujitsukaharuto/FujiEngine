#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Vector/Vector4.h"

namespace GameObject {
	class GameObject;
	class PlacedObject;
	class SpawnPoint;
}
namespace Graphics {
	class PlacedSprite;
	class SkyBox;
	class LightManager;
}

namespace Scene {

	/// <summary>
	/// シーン全体の見た目。空と平行光源とカメラの初期位置
	/// </summary>
	/// <remarks>配置データの "environment"。シーンに入るたびにまるごと適用するので、前のシーンの設定は持ち越さない</remarks>
	struct Environment {
		bool hasSkyBox = false;
		Math::Vector4 skyBoxColor = { 1.0f,1.0f,1.0f,1.0f };
		Math::Vector4 lightColor = { 1.0f,1.0f,1.0f,1.0f };
		Math::Vector3 lightDirection = { 0.0f,-1.0f,0.0f };
		float lightIntensity = 1.0f;
		/// <summary>シーンに入ったときのカメラ。以降はシーンが自由に動かしてよい</summary>
		Math::Vector3 cameraPosition = { 0.0f,5.0f,-30.0f };
		/// <summary>ラジアン</summary>
		Math::Vector3 cameraRotate = { 0.0f,0.0f,0.0f };
	};

	/// <summary>
	/// シーンに置く置物・出現位置・スプライトの一覧。配置データ(resource/Json/Level/シーン名.json)と対応する
	/// </summary>
	/// <remarks>Release でも読むので、地形や背景をコードに書かずに置ける。編集と保存は Hierarchy から行う</remarks>
	class Level {
	public:
		Level();
		~Level();

		/// <summary>配置データを読み、置物を生成して環境を適用する。ファイルが無ければ空で、環境は既定値</summary>
		/// <remarks>ライトは点光源・スポットライトも含めて起動時の状態へ戻してから平行光源を入れる</remarks>
		void Load(const std::string& name, Graphics::LightManager* lightManager);
		/// <summary>今の置物を配置データへ書き出す</summary>
		void Save() const;

		void Update();
		/// <summary>置物を描く</summary>
		void Draw();
		/// <summary>スプライトを並び順に描く</summary>
		/// <remarks>SceneManager がシーンの Draw の後に呼ぶ。シーンやオブジェクトが描くスプライト(HPバー等)より手前に出る</remarks>
		void DrawSprites();

		/// <summary>置物を1つ追加する</summary>
		GameObject::PlacedObject* Add(const std::string& modelName);
		/// <summary>置物か出現位置を破棄する</summary>
		/// <returns>この一覧のものだったら true</returns>
		bool Remove(const GameObject::GameObject* object);
		/// <summary>この一覧の置物か出現位置か</summary>
		bool Contains(const GameObject::GameObject* object) const;
		/// <summary>名前で置物を探す。無ければ nullptr</summary>
		/// <remarks>同じ名前が複数あれば先頭を返す</remarks>
		GameObject::PlacedObject* Find(const std::string& name);

		//========================================================================*/
		//* 出現位置
		/// <summary>出現位置を1つ追加する</summary>
		GameObject::SpawnPoint* AddSpawnPoint();
		/// <summary>入口の名前に合う出現位置</summary>
		/// <remarks>名前が空か、合うものが無ければ先頭を返す。1つも無ければ nullptr</remarks>
		const GameObject::SpawnPoint* FindSpawnPoint(const std::string& entry) const;

		//========================================================================*/
		//* スプライト
		/// <summary>スプライトを1枚追加する。大きさはテクスチャのまま</summary>
		Graphics::PlacedSprite* AddSprite(const std::string& texture);
		/// <returns>この一覧のスプライトだったら true</returns>
		bool RemoveSprite(const Graphics::PlacedSprite* sprite);
		bool ContainsSprite(const Graphics::PlacedSprite* sprite) const;
		/// <summary>名前でスプライトを探す。無ければ nullptr</summary>
		/// <remarks>シーンは Initialize でこれを呼んでポインタを控えておけばよい。スプライトはシーンが終わるまで消えない(エディタで Remove しない限り)</remarks>
		Graphics::PlacedSprite* FindSprite(const std::string& name);
		/// <summary>描く順(後ろほど手前)</summary>
		const std::vector<std::unique_ptr<Graphics::PlacedSprite>>& GetSprites() const { return sprites_; }

		//========================================================================*/
		//* 環境
		const Environment& GetEnvironment() const { return environment_; }
		/// <summary>Inspector に出す環境の編集欄。触った値はその場で反映する</summary>
		void EnvironmentGUI();

		/// <summary>配置データの名前(シーンの登録名)</summary>
		const std::string& GetName() const { return name_; }

	private:

		/// <summary>environment_ をスカイボックスとライトへ入れる</summary>
		void ApplyEnvironment();
		/// <summary>environment_ のカメラの初期位置をカメラへ入れる</summary>
		/// <remarks>ApplyEnvironment と分けてあるのは、空やライトを編集したときにシーンが動かしているカメラを巻き戻さないため</remarks>
		void ApplyCamera();

	private:

		std::string name_;
		Environment environment_;
		/// <summary>一度作ったら Level が消えるまで持つ。描画中のバッファを途中で捨てないため</summary>
		std::unique_ptr<Graphics::SkyBox> skyBox_;
		Graphics::LightManager* lightManager_ = nullptr;
		std::vector<std::unique_ptr<GameObject::PlacedObject>> objects_;
		std::vector<std::unique_ptr<GameObject::SpawnPoint>> spawnPoints_;
		std::vector<std::unique_ptr<Graphics::PlacedSprite>> sprites_;
	};

}
