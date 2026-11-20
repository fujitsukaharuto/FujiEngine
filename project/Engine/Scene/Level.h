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
	class GIVolume;
	class ReflectionProbe;
	class LocalFogVolume;
	class PostProcessVolume;
	class PlacedSpline;
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
	/// <remarks>配置データの "environment"。シーンに入るたびにまるごと適用する</remarks>
	struct Environment {
		bool hasSkyBox = false;
		Math::Vector4 skyBoxColor = { 1.0f,1.0f,1.0f,1.0f };
		/// <summary>Graphics::SkyMode の値。0:Texture 1:Atmosphere</summary>
		int skyMode = 0;
		/// <summary>Atmosphere の空の明るさ</summary>
		float skyIntensity = 1.0f;
		/// <summary>Atmosphere の空気遠近の距離の倍率。1 で実際の濃さ、0 で掛けない</summary>
		float aerialPerspectiveScale = 1.0f;
		/// <summary>Atmosphere の空気遠近を3Dの表 (UE の Aerial Perspective LUT) から引く</summary>
		/// <remarks>false ならカメラの高さの空気が一様とみなす式</remarks>
		bool aerialPerspectiveLut = true;
		/// <summary>Atmosphere の空で、カメラの高さに足す高度 (km)。100 を超えると大気の外から星を見下ろす</summary>
		float atmosphereAltitude = 0.0f;
		/// <summary>Atmosphere の星と大気の大きさの倍率。小さくすると低い所で宇宙に出る (地表から見た空の色は同じ)</summary>
		float atmosphereScale = 1.0f;
		/// <summary>Atmosphere のとき、空から回り込む光の強さ</summary>
		float skyLightIntensity = 1.0f;
		/// <summary>Atmosphere の雲の量。0 で無し、1 でほぼ曇り</summary>
		float cloudCoverage = 0.0f;
		/// <summary>雲が地面に落とす影の強さ (0〜1)</summary>
		float cloudShadow = 0.0f;
		/// <summary>Atmosphere の星の明るさ。0で描かない</summary>
		float stars = 0.0f;
		/// <summary>Atmosphere の月 (太陽の反対側) の明るさ。0で描かない</summary>
		float moon = 0.0f;
		/// <summary>街の光で空の下の方が染まる色 (光害)</summary>
		Math::Vector3 skyGlowColor = { 1.0f,0.45f,0.25f };
		/// <summary>光害の明るさ。0で掛からない</summary>
		float skyGlow = 0.0f;
		/// <summary>光害が届く高さ。地平線から真上を1とした向きの y</summary>
		float skyGlowHeight = 0.25f;
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
		/// <summary>太陽の方向のフォグを明るくする強さ。0で掛からない</summary>
		float fogSunScatter = 0.0f;
		/// <summary>大きいほど太陽のまわりだけが明るくなる</summary>
		float fogSunPower = 8.0f;
		/// <summary>2層目の霧の濃さ。0で掛からない。色・始まる距離・上限は1層目と同じ</summary>
		/// <remarks>UE の Second Fog Data。地面すれすれの濃い霧と上空の薄い霧を分けて持てる</remarks>
		float fogSecondDensity = 0.0f;
		/// <summary>2層目の高さによる薄まり方</summary>
		float fogSecondHeightFalloff = 0.0f;
		/// <summary>2層目の基準の高さ。1層目の基準の高さからのずれ</summary>
		float fogSecondHeightOffset = 0.0f;

		// ボリュメトリックフォグ (UE の Volumetric Fog)。カメラの近くの霧を箱に分け、光と影ごと求める。
		// 霧の濃さ・始まる距離は上の Fog と同じで、表の届く距離より先は Fog の色の霧のまま。On の間は光の筋 (volumetric*) は止まる
		bool volumetricFog = false;
		/// <summary>散乱 / 消散。1 で霧が光を吸わない</summary>
		Math::Vector3 volumetricFogAlbedo = { 1.0f,1.0f,1.0f };
		/// <summary>霧が自分で出す光 (1mあたり)</summary>
		Math::Vector3 volumetricFogEmissive = { 0.0f,0.0f,0.0f };
		/// <summary>霧の濃さに掛ける倍率</summary>
		float volumetricFogExtinctionScale = 1.0f;
		/// <summary>散乱の向きの偏り。0 で全方向へ同じ、1 に近いほど光源の側だけ明るい</summary>
		float volumetricFogScatteringDistribution = 0.2f;
		/// <summary>表が届く奥行き (m)</summary>
		float volumetricFogViewDistance = 60.0f;
		/// <summary>平行光源・空・ライトの光の倍率</summary>
		float volumetricFogSun = 1.0f;
		float volumetricFogSkyLight = 1.0f;
		float volumetricFogLocalLights = 1.0f;
		/// <summary>前のフレームと混ぜて粒を均す。切ると箱の模様が出る</summary>
		bool volumetricFogTemporal = true;

		// 画面の仕上げ(ブルームとトーンマップ)。既定値は Graphics::PostProcessSettings と同じ
		bool bloomEnabled = true;
		float bloomThreshold = 0.75f;
		float bloomIntensity = 1.0f;
		float bloomKnee = 0.1f;
		float bloomScatter = 0.7f;
		/// <summary>Graphics::TonemapMode の値</summary>
		int tonemapMode = 2;
		float exposure = 2.0f;
		/// <summary>自動露出。exposure はその上に掛かる補正になる</summary>
		bool autoExposure = false;
		/// <summary>自動露出で暗くする限度 (EV)。-1 で半分まで</summary>
		float autoExposureMinEV = -1.0f;
		/// <summary>自動露出で明るくする限度 (EV)。3 で8倍まで</summary>
		float autoExposureMaxEV = 3.0f;
		/// <summary>画面が明るくなったときに慣れる速さ</summary>
		float autoExposureSpeedUp = 3.0f;
		/// <summary>画面が暗くなったときに慣れる速さ</summary>
		float autoExposureSpeedDown = 1.0f;
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

		// 描画の重さに効く設定。Hierarchy > Environment の Quality のボタンでまとめて切り替えられる
		/// <summary>Graphics::RayTracedAOMode の値。0:Off 1:Inline 2:Screen</summary>
		int aoMode = 2;
		/// <summary>影なしのライトの拡散光にも AO を掛ける。接地の影が出る</summary>
		bool localLightAO = false;
		/// <summary>Graphics::RayTracedShadowMode の値。0:Hard 1:Soft</summary>
		int shadowMode = 0;
		/// <summary>大きさ (sourceRadius) のある点光源とスポットの影を柔らかくする。近い3本まで</summary>
		bool softLocalShadow = false;
		/// <summary>Graphics::AmbientMode の値。0:Hemisphere 1:IBL</summary>
		int ambientMode = 0;
		/// <summary>IBL のときだけアンビエントに掛ける倍率。Hemisphere と近い明るさになる値を既定にしてある</summary>
		float iblAmbientScale = 0.6f;
		/// <summary>Graphics::LightingMode の値。0:Simple 1:PBR</summary>
		int lightingMode = 0;
		/// <summary>鏡面の細部 (多重散乱の補正、映り込みの遮蔽)</summary>
		bool specularDetail = false;
		/// <summary>ハイライトのちらつきを粗さで抑える</summary>
		bool specularAA = false;
		/// <summary>映り込みを付けていないマテリアルにも付ける</summary>
		bool reflectAll = false;
		/// <summary>プローブの間接光</summary>
		bool gi = false;
		/// <summary>プローブの間隔(m)</summary>
		float giProbeSpacing = 3.0f;
		/// <summary>プローブ1つあたりのレイの本数(32の倍数)</summary>
		int giRays = 64;
		/// <summary>画面のプローブの間接光。gi の上に近くの物陰と跳ね返りを足す</summary>
		bool giScreen = false;
		/// <summary>画面の照り返し (SSGI)。画面に写っている物の色を近くの面へ足す</summary>
		bool ssgi = false;
		/// <summary>画面の照り返しに掛ける倍率</summary>
		float ssgiIntensity = 1.0f;
		/// <summary>画面の照り返しのレイの届く距離 (m)</summary>
		float ssgiDistance = 3.0f;

		// 鏡面の映り込み。映り込ませるマテリアル(environment が0より大きい物)にだけ効く
		/// <summary>Graphics::RayTracedReflectionMode の値</summary>
		int reflectionMode = 0;
		/// <summary>Objects のとき、映り込みを別パスで求めて粗さに応じてぼかす</summary>
		bool reflectionDenoised = false;
		/// <summary>別パスの映り込みで、2回目の跳ね返りも出す</summary>
		bool reflectionBounce = false;
		/// <summary>別パスの映り込みで、画面に写っている所は画面の色を使う (SSR)</summary>
		bool reflectionSceneColor = false;
		/// <summary>別パスの映り込みを、浅い角度ほど縦に伸ばす強さ。0で伸ばさない</summary>
		float reflectionStretch = 0.0f;
		/// <summary>この距離までの物を見る</summary>
		float reflectionMaxDistance = 100.0f;
		float reflectionIntensity = 1.0f;
		/// <summary>平行光源の影の中で映り込みをどれだけ暗くするか(0〜1)</summary>
		float reflectionShadow = 0.5f;

		// 濡れた路面
		/// <summary>全体の濡れ具合(0〜1)。0で掛からない</summary>
		float wetness = 0.0f;
		/// <summary>水たまりの多さ(0〜1)。濡れ具合が半分を超えてから溜まる</summary>
		float puddleAmount = 0.5f;
		/// <summary>水たまりの大きさ(m)</summary>
		float puddleScale = 4.0f;
		/// <summary>雨の強さ(0〜1)。雨粒と波紋が増える</summary>
		float rain = 0.0f;
		/// <summary>地面のしぶきの量(0〜1)。0で出ない</summary>
		float rainSplash = 0.0f;
		/// <summary>庇・看板・電線などから垂れる雨だれの量(0〜1)。0で出ない</summary>
		float rainDrip = 0.0f;
		/// <summary>遠くの雨のカーテンの濃さ(0〜1)。0で出ない</summary>
		float rainCurtain = 0.0f;
		/// <summary>走る車の水はねの量(0〜1)。0で出ない</summary>
		float rainSpray = 0.0f;

		// 被写界深度。ピントの合う距離から離れた物をぼかす
		/// <summary>ぼけの最大半径(画素)。0で掛からない</summary>
		float dofBlur = 0.0f;
		/// <summary>ピントの合う、カメラからの距離</summary>
		float dofFocusDistance = 10.0f;
		/// <summary>ピントの前後、この幅まではぼかさない</summary>
		float dofFocusRange = 5.0f;
		/// <summary>そこからこの距離をかけて最大のぼけに達する</summary>
		float dofFalloff = 20.0f;

		// レンズと色味。どれも0で掛からない
		/// <summary>色収差の強さ</summary>
		float chromaticAberration = 0.0f;
		/// <summary>太陽のレンズフレアの強さ</summary>
		float lensFlare = 0.0f;
		/// <summary>モーションブラーのシャッターの開いている割合</summary>
		float motionBlur = 0.0f;
		/// <summary>半透明の屈折で、面の奥のこの距離(m)の物をずらして見せる。0で掛からない</summary>
		float refraction = 0.0f;
		/// <summary>光の筋 (ボリュメトリックライト) の 1m あたりの散乱。0で掛からない</summary>
		float volumetricDensity = 0.0f;
		/// <summary>散乱の向きの偏り。1に近いほど太陽の側だけ明るい</summary>
		float volumetricAnisotropy = 0.6f;
		/// <summary>カメラからこの距離までの空気を光らせる</summary>
		float volumetricDistance = 50.0f;
		/// <summary>散った光の倍率。霞を濃くせず筋だけ明るくする</summary>
		float volumetricIntensity = 1.0f;
		/// <summary>太陽の光の筋の倍率。0で影のレイも飛ばさない</summary>
		float volumetricSun = 1.0f;
		/// <summary>影なしの配置ライトが霧でにじむ倍率</summary>
		float volumetricLocalLights = 0.0f;
		/// <summary>色温度。正で暖色(夕方)、負で寒色</summary>
		float whiteBalanceTemperature = 0.0f;
		/// <summary>正でマゼンタ、負で緑へ寄せる</summary>
		float whiteBalanceTint = 0.0f;
		/// <summary>暗い所ほど青く見せる (プルキンエ現象) 強さ</summary>
		float purkinje = 0.0f;
		/// <summary>この明るさ (露出後) より暗い所から青く寄せる</summary>
		float purkinjeLuminance = 0.05f;
		/// <summary>局所トーンマップの強さ (0〜1)。暗部と明部を中間へ寄せる</summary>
		float localTonemap = 0.0f;
		/// <summary>局所トーンマップで残す細部の明暗。1でそのまま</summary>
		float localTonemapDetail = 1.0f;
		/// <summary>フィルムの粒子の強さ。0で掛からない</summary>
		float filmGrain = 0.0f;
		/// <summary>黒を浮かせて褪せたフィルムのようにする強さ</summary>
		float fade = 0.0f;
	};

	/// <summary>描画の品質の段。右ほど豪華で重い。Standard が普段の絵</summary>
	enum class Quality {
		Standard,
		High,
		Ultra,
		Realistic,
		Count,
	};

	/// <summary>段の名前。GUI のボタンと環境変数 FUJI_QUALITY で使う</summary>
	const char* GetQualityName(Quality quality);
	/// <summary>段に合わせて、描画の重さに効く項目だけを書き換える</summary>
	/// <remarks>霧・濡れ・雨・光の筋などの絵作りの値と、PBR の切り替えは触らない</remarks>
	void ApplyQuality(Environment& env, Quality quality);

	/// <summary>時刻のキー。時刻で変える Environment の項目の値を持つ</summary>
	struct TimeOfDayKey {
		float hour = 12.0f;
		/// <summary>並びは Level.cpp の kTimeOfDayParams</summary>
		std::vector<float> values;
	};

	/// <summary>
	/// 一日の時刻で環境を変える。太陽の向きは時刻から決める
	/// </summary>
	/// <remarks>配置データの environment の "timeOfDay"</remarks>
	struct TimeOfDay {
		bool enabled = false;
		/// <summary>今の時刻 (0〜24)</summary>
		float hour = 12.0f;
		/// <summary>1秒で進む時間 (時)。0で止まる</summary>
		float hoursPerSecond = 0.0f;
		/// <summary>平行光源の向きを時刻から決める。夜は月の側から照らす</summary>
		bool sunFromTime = true;
		/// <summary>日の昇る方角 (度)。0 で +X、90 で +Z から昇る</summary>
		float sunAzimuth = 0.0f;
		/// <summary>正午の太陽の高さ (度)</summary>
		float sunMaxElevation = 60.0f;
		/// <summary>月の光の強さ。平行光源の強さに掛ける</summary>
		float moonIntensity = 0.15f;
		/// <summary>月の光の色。平行光源の色に掛ける</summary>
		Math::Vector3 moonColor = { 0.55f,0.65f,1.0f };
		/// <summary>夜の空・アンビエント・フォグの色の明るさ (昼が1)</summary>
		float nightBrightness = 0.08f;
		/// <summary>夜に自動露出が明るくする限度 (EV)</summary>
		float nightMaxEV = 1.0f;
		/// <summary>夜のプルキンエの強さ。Environment の値の方が大きければそちら</summary>
		float nightPurkinje = 0.6f;
		/// <summary>時刻で変える項目の名前 (kTimeOfDayParams の name)</summary>
		std::vector<std::string> params;
		std::vector<TimeOfDayKey> keys;
	};

	/// <summary>
	/// シーンに置く置物・出現位置・スプライトの一覧。配置データ(resource/Json/Level/シーン名.json)と対応する
	/// </summary>
	/// <remarks>Release でも読む。編集と保存は Hierarchy から行う</remarks>
	class Level {
	public:
		Level();
		~Level();

		/// <summary>配置データを読み、置物を生成して環境を適用する。ファイルが無ければ空で、環境は既定値</summary>
		/// <remarks>ライトは点光源・スポットライトも含めて起動時の状態へ戻してから平行光源を入れる。環境変数 FUJI_QUALITY に段の名前があればその段で、FUJI_ENV に JSON があればその値で上書きする (計測用)</remarks>
		void Load(const std::string& name, Graphics::LightManager* lightManager);
		/// <summary>今の置物を配置データへ書き出す</summary>
		void Save() const;

		void Update();
		/// <summary>置物を描く</summary>
		void Draw();
		/// <summary>スプライトを並び順に描く</summary>
		/// <remarks>SceneManager がシーンの Draw の後に呼ぶので、シーンが描くスプライトより手前に出る</remarks>
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
		/// <summary>形のある影なしの光源 (棒か長方形) を1つ追加する</summary>
		GameObject::PlacedLight* AddShapedLight(bool isTube);

		//========================================================================*/
		//* 間接光を焼く箱
		GameObject::GIVolume* AddGIVolume();

		//========================================================================*/
		//* 反射プローブの箱
		GameObject::ReflectionProbe* AddReflectionProbe();

		//========================================================================*/
		//* 局所の霧
		/// <summary>局所の霧を1つ追加する。上限まで置いていたら nullptr</summary>
		GameObject::LocalFogVolume* AddLocalFogVolume();

		//========================================================================*/
		//* ポストプロセスボリューム
		/// <summary>ポストプロセスボリュームを1つ追加する</summary>
		GameObject::PostProcessVolume* AddPostProcessVolume();

		//========================================================================*/
		//* 曲線
		/// <summary>曲線を1つ追加する。原点から +Z へ10mの直線</summary>
		GameObject::PlacedSpline* AddSpline();
		/// <summary>名前で曲線を探す。無ければ nullptr</summary>
		/// <remarks>ゲームは GetWorldSpline() の距離で位置と向きを取る (カメラのレール・敵の通り道など)</remarks>
		const GameObject::PlacedSpline* FindSpline(const std::string& name) const;

		//========================================================================*/
		//* スプライト
		/// <summary>スプライトを1枚追加する。大きさはテクスチャのまま</summary>
		Graphics::PlacedSprite* AddSprite(const std::string& texture);
		/// <returns>この一覧のスプライトだったら true</returns>
		bool RemoveSprite(const Graphics::PlacedSprite* sprite);
		bool ContainsSprite(const Graphics::PlacedSprite* sprite) const;
		/// <summary>名前でスプライトを探す。無ければ nullptr</summary>
		/// <remarks>シーンは Initialize でポインタを控えておけばよい。エディタで Remove しない限り消えない</remarks>
		Graphics::PlacedSprite* FindSprite(const std::string& name);
		/// <summary>描く順(後ろほど手前)</summary>
		const std::vector<std::unique_ptr<Graphics::PlacedSprite>>& GetSprites() const { return sprites_; }

		//========================================================================*/
		//* 環境
		const Environment& GetEnvironment() const { return environment_; }
		const TimeOfDay& GetTimeOfDay() const { return timeOfDay_; }
		/// <summary>時刻を変えて、その時刻の環境を当てる</summary>
		void SetHour(float hour);
		/// <summary>品質の段を当てる。配置データには Save するまで書かない</summary>
		void SetQuality(Quality quality);
		/// <summary>Inspector に出す環境の編集欄。触った値はその場で反映する</summary>
		void EnvironmentGUI();

		/// <summary>配置データの名前(シーンの登録名)</summary>
		const std::string& GetName() const { return name_; }

	private:

		/// <summary>environment_ にポストプロセスボリュームを混ぜた値を、スカイボックスとライトへ入れる</summary>
		/// <remarks>environment_ (保存する値) は書き換えない</remarks>
		void ApplyEnvironment();
		/// <summary>environment_ をそのままスカイボックスとライトへ入れる</summary>
		void ApplyEnvironmentRaw();
		/// <summary>カメラの位置で、ポストプロセスボリュームを優先度の低い順に env へ混ぜる</summary>
		/// <param name="weights">箱ごとの混ぜた割合 (置いた順)</param>
		void BlendPostProcessVolumes(Environment& env, std::vector<float>& weights) const;
		/// <summary>environment_ のカメラの初期位置をカメラへ入れる</summary>
		/// <remarks>空やライトの編集でカメラを巻き戻さないよう、ApplyEnvironment と分けてある</remarks>
		void ApplyCamera();
		/// <summary>今の時刻のキーの値と太陽の向きを environment_ へ書く。ApplyEnvironment の前に呼ぶ</summary>
		void ApplyTimeOfDay();
		/// <summary>時刻を進める。進んだら環境を当て直す</summary>
		void UpdateTimeOfDay();
		/// <summary>Environment の編集欄のうち、焼いた間接光の欄</summary>
		/// <returns>環境を当て直す必要がある</returns>
		bool BakedGIGUI();
		/// <summary>置いた箱で間接光を焼き始める</summary>
		void StartBakeGI();
		/// <summary>焼いた値を置く場所 (resource/GI/シーン名)</summary>
		std::string GetBakedGIDirectory() const;
		/// <summary>置いた反射プローブの箱を描画へ渡す</summary>
		void ApplyReflectionProbes() const;
		/// <summary>置いた局所の霧を描画へ渡す</summary>
		void ApplyLocalFogVolumes() const;
		/// <summary>Environment の編集欄のうち、反射プローブの欄</summary>
		void ReflectionProbeGUI();

		/// <summary>Environment の編集欄のうち、時刻の欄</summary>
		/// <returns>時刻かキーが変わり、環境を当て直す必要がある</returns>
		bool TimeOfDayGUI();

	private:

		std::string name_;
		Environment environment_;
		TimeOfDay timeOfDay_;
		/// <summary>時刻から太陽を決めるとき、ApplyEnvironment が environment_ の値に掛けるもの</summary>
		/// <remarks>environment_ を書き換えると掛けるたびに値が小さくなっていくので別に持つ</remarks>
		struct TimeOfDayLighting {
			/// <summary>平行光源の強さの倍率。太陽と月が入れ替わる前後は0</summary>
			float lightScale = 1.0f;
			/// <summary>平行光源の色の倍率。夜は月の色</summary>
			Math::Vector3 lightTint = { 1.0f,1.0f,1.0f };
			/// <summary>テクスチャの空・半球アンビエント・フォグの色の倍率</summary>
			float skyScale = 1.0f;
			/// <summary>空を計算するときの夜の明るさの下限</summary>
			Math::Vector3 nightAmbient = { 0.0f,0.0f,0.0f };
			/// <summary>夜の度合い。0 で昼、1 で薄明の終わった夜</summary>
			float night = 0.0f;
			/// <summary>空に渡す本当の太陽。夜は地平線の下にある</summary>
			bool hasSkySun = false;
			Math::Vector3 toSun = { 0.0f,1.0f,0.0f };
		};
		TimeOfDayLighting timeLighting_;
		// 時刻の欄の編集の状態。保存しない
		int selectedKey_ = -1;
		int draggingKey_ = -1;
		int curveParam_ = 0;
		bool isTimePaused_ = false;
		/// <summary>一度作ったら Level が消えるまで持つ。描画中のバッファを途中で捨てないため</summary>
		std::unique_ptr<Graphics::SkyBox> skyBox_;
		Graphics::LightManager* lightManager_ = nullptr;
		std::vector<std::unique_ptr<GameObject::PlacedObject>> objects_;
		std::vector<std::unique_ptr<GameObject::SpawnPoint>> spawnPoints_;
		std::vector<std::unique_ptr<GameObject::PlacedLight>> lights_;
		std::vector<std::unique_ptr<GameObject::GIVolume>> giVolumes_;
		std::vector<std::unique_ptr<GameObject::ReflectionProbe>> reflectionProbes_;
		std::vector<std::unique_ptr<GameObject::LocalFogVolume>> localFogVolumes_;
		std::vector<std::unique_ptr<GameObject::PostProcessVolume>> postProcessVolumes_;
		std::vector<std::unique_ptr<GameObject::PlacedSpline>> splines_;
		// 最後に当てたときのポストプロセスボリュームの割合。変わったら当て直す
		std::vector<float> appliedVolumeWeights_;
		std::vector<std::unique_ptr<Graphics::PlacedSprite>> sprites_;
	};

}
