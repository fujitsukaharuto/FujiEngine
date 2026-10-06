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
	class PlacedLight;
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

		// 空気(フォグ)。遠くの物ほどこの色へ寄せる。濃さが0なら掛からない
		Math::Vector3 fogColor = { 0.6f,0.7f,0.8f };
		/// <summary>フォグの色を空の地平線から取る。空の色を変えるとフォグも付いてくる。空が無ければ fogColor を使う</summary>
		bool fogColorFromSky = false;
		/// <summary>空から取った色に掛ける倍率。1で地平線と同じ色</summary>
		float fogSkyColorScale = 1.0f;
		float fogDensity = 0.0f;
		/// <summary>高さによる薄まり方。0なら高さに依らず一様</summary>
		float fogHeightFalloff = 0.0f;
		/// <summary>濃さの基準にする高さ</summary>
		float fogBaseHeight = 0.0f;
		/// <summary>カメラからこの距離までは霞ませない</summary>
		float fogStartDistance = 0.0f;
		float fogMaxOpacity = 1.0f;
		/// <summary>空へどれだけ掛けるか</summary>
		float fogSkyAmount = 1.0f;

		// 画面の仕上げ(ブルームとトーンマップ)。既定値は Graphics::PostProcessSettings と同じ
		bool bloomEnabled = true;
		float bloomThreshold = 0.75f;
		float bloomIntensity = 1.0f;
		float bloomKnee = 0.1f;
		float bloomScatter = 0.7f;
		/// <summary>Graphics::TonemapMode の値。0:None 1:Reinhard 2:ACES 3:AgX</summary>
		int tonemapMode = 2;
		float exposure = 2.0f;
		float saturation = 1.0f;
		float contrast = 1.0f;
		/// <summary>輪郭をくっきりさせる強さ。0で掛からない</summary>
		float sharpen = 0.0f;
		/// <summary>画面の四隅を暗くする強さ。0で掛からない</summary>
		float vignette = 0.0f;
		/// <summary>暗い所と明るい所に掛ける色。白で掛からない</summary>
		Math::Vector3 shadowTint = { 1.0f,1.0f,1.0f };
		Math::Vector3 highlightTint = { 1.0f,1.0f,1.0f };

		// 光の筋(ゴッドレイ)。平行光源の来る方を向くと、物の隙間から光が伸びる。色と向きは平行光源のもの
		/// <summary>0で掛からない</summary>
		float godRayIntensity = 0.0f;
		/// <summary>筋の長さ。1で太陽まで届く</summary>
		float godRayLength = 0.6f;
		/// <summary>光源とみなす太陽のまわりの広さ。画面の高さが1</summary>
		float godRaySunSize = 0.4f;

		// 鏡面の映り込み。映り込ませるマテリアル(environment が0より大きい物)にだけ効く
		/// <summary>Graphics::RayTracedReflectionMode の値。0:Off(どの向きにも空が映る) 1:Occlusion(遮られた向きは空を映さない) 2:Objects(遮った物の色を映す)</summary>
		int reflectionMode = 0;
		/// <summary>この距離までの物を見る</summary>
		float reflectionMaxDistance = 100.0f;
		float reflectionIntensity = 1.0f;

		// 被写界深度。ピントの合う距離から離れた物をぼかす
		/// <summary>ぼけの最大半径(画素)。0で掛からない</summary>
		float dofBlur = 0.0f;
		/// <summary>ピントの合う、カメラからの距離</summary>
		float dofFocusDistance = 10.0f;
		/// <summary>ピントの前後、この幅まではぼかさない</summary>
		float dofFocusRange = 5.0f;
		/// <summary>そこからこの距離をかけて最大のぼけに達する</summary>
		float dofFalloff = 20.0f;
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
		/// <summary>置物・出現位置・ライトを同じ設定でもう1つ作る</summary>
		/// <returns>作ったもの。この一覧のものでなければ nullptr</returns>
		GameObject::GameObject* Duplicate(const GameObject::GameObject* source);
		/// <summary>置物・出現位置・ライトを破棄する</summary>
		/// <returns>この一覧のものだったら true</returns>
		bool Remove(const GameObject::GameObject* object);
		/// <summary>この一覧の置物・出現位置・ライトか</summary>
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
		//* ライト
		/// <summary>点光源かスポットライトを1つ追加する</summary>
		GameObject::PlacedLight* AddLight(bool isSpot);

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
		std::vector<std::unique_ptr<GameObject::PlacedLight>> lights_;
		std::vector<std::unique_ptr<Graphics::PlacedSprite>> sprites_;
	};

}
