#pragma once
#include "Engine/Graphics/Light/DirectionLight.h"
#include "Engine/Graphics/Light/PointLight.h"
#include "Engine/Graphics/Light/SpotLight.h"
#include "Engine/Graphics/SkyBox/SkyAtmosphere.h"
#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl.h>

namespace DXC { class DXCom; }

namespace Graphics {

	const int kMaxDirectionalLights = 3;
	const int kMaxPointLights = 10;
	const int kMaxSpotLights = 10;
	/// <summary>影を落とさないライトの上限 (クラスタ化ライティング)</summary>
	const int kMaxClusteredLights = 1024;

	/// <summary>ClusteredLightData::type の値。HLSL 側 (Common/ClusteredLights.hlsli) と同じ値であること</summary>
	enum ClusteredLightType : uint32_t {
		kClusteredLightPoint = 0,
		kClusteredLightSpot = 1,
		kClusteredLightTube = 2,	// 棒。direction が軸の向き × 長さの半分
		kClusteredLightRect = 3,	// 長方形。direction は横×幅の半分、縦×高さの半分は cosAngle/cosFalloffStart/pad[0]
	};

	/// <summary>
	/// 影を落とさないライト1つ
	/// </summary>
	/// <remarks>HLSL 側 (Common/ClusteredLights.hlsli の ClusteredLight) と並びが一致していること</remarks>
	struct ClusteredLightData {
		Math::Vector3 position{};
		float range = 10.0f;			// 光が届く距離
		Math::Vector3 color{};			// 色 × 強さ。0 なら消灯
		float decay = 1.0f;				// 距離による減り方
		Math::Vector3 direction = { 0.0f,-1.0f,0.0f };	// スポットライトの照らす向き
		float cosAngle = 0.5f;			// スポットライトの外側の角度の cos
		float cosFalloffStart = 0.6f;	// 内側の角度の cos
		uint32_t type = kClusteredLightPoint;
		float pad[2] = {};				// 長方形: [0] は縦の軸の z、[1] が 1 なら両面
	};
	static_assert(sizeof(ClusteredLightData) == 64, "ClusteredLightData の大きさが ClusteredLights.hlsli と一致していない");

	/// <summary>
	/// レイトレ影を光源の種類ごとに切るビット
	/// </summary>
	/// <remarks>HLSL 側 (Common/RayTracedShadow.hlsli) の kShadowMask* と同じ値であること</remarks>
	enum RayTracedShadowMask : uint32_t {
		kShadowMaskDirectional = 1u << 0,
		kShadowMaskPoint = 1u << 1,
		kShadowMaskSpot = 1u << 2,
		kShadowMaskAll = kShadowMaskDirectional | kShadowMaskPoint | kShadowMaskSpot,
	};

	/// <summary>
	/// 平行光源の影をどの経路で計算するか
	/// </summary>
	/// <remarks>HLSL 側 (Common/RayTracedShadow.hlsli) の kShadowMode* と同じ値であること</remarks>
	enum RayTracedShadowMode : uint32_t {
		kShadowModeHard = 0,	// 前方描画のPSで1本だけ飛ばす。輪郭が硬い代わりに軽い
		kShadowModeSoft = 1,	// 別パスで光源の広がりぶん散らして飛ばし、デノイズする
	};

	/// <summary>
	/// アンビエント(直接光以外の明るさ)をどこから取るか
	/// </summary>
	/// <remarks>HLSL 側 (Common/IBL.hlsli) の kAmbientMode* と同じ値であること</remarks>
	enum AmbientMode : uint32_t {
		kAmbientModeHemisphere = 0,	// 空色と地面色の2色補間。焼かずに済む代わりに環境の色は乗らない
		kAmbientModeIBL = 1,		// 環境マップから焼いた irradiance / prefiltered を引く
	};

	/// <summary>
	/// 空の描き方
	/// </summary>
	/// <remarks>HLSL 側 (Common/Atmosphere.hlsli) の kSkyMode* と同じ値であること</remarks>
	enum SkyMode : uint32_t {
		kSkyModeTexture = 0,		// キューブマップを貼る
		kSkyModeAtmosphere = 1,		// 0番の平行光源の向きから計算する
	};

	/// <summary>
	/// 直接光の計算式
	/// </summary>
	/// <remarks>HLSL 側 (Common/SimpleLighting.hlsli) の kLightingMode* と同じ値であること</remarks>
	enum LightingMode : uint32_t {
		kLightingModeSimple = 0,	// Lambert + Blinn-Phong
		kLightingModePBR = 1,		// Cook-Torrance
	};

	/// <summary>
	/// レイトレAOをどの経路で計算するか
	/// </summary>
	/// <remarks>HLSL 側 (Common/RayTracedAO.hlsli) の kAOMode* と同じ値であること</remarks>
	enum RayTracedAOMode : uint32_t {
		kAOModeOff = 0,		// 計算しない。G-Bufferのプリパスごと止まる
		kAOModeInline = 1,	// 前方描画のPSで直接レイを飛ばす。デノイズできないので比較用
		kAOModeScreen = 2,	// 別パスで計算しデノイズしたものを画面空間で引く
	};

	/// <summary>
	/// 鏡面の映り込みで、反射方向にある物をどう扱うか
	/// </summary>
	/// <remarks>HLSL 側 (Common/RayTracedReflection.hlsli) の kReflectionMode* と同じ値であること</remarks>
	enum RayTracedReflectionMode : uint32_t {
		kReflectionModeOff = 0,			// 遮蔽を見ない。どの向きにも空が映る(レイトレ導入前と同じ絵)
		kReflectionModeOcclusion = 1,	// 物に遮られた向きには空を映さない
		kReflectionModeObjects = 2,		// 遮った物の色を映す。いちばん手前の物を探すので Occlusion より重い
	};

	/// <summary>
	/// 品質の設定で足す描画の細部。ビットごとに切り替える
	/// </summary>
	/// <remarks>HLSL 側 (Common/IBL.hlsli) の kRenderFlag* と同じ値であること</remarks>
	enum RenderFlag : uint32_t {
		kRenderFlagEnergyCompensation = 1u << 0,	// 多重散乱のぶんを足す
		kRenderFlagSpecularOcclusion = 1u << 1,	// AO と法線マップの地平線で映り込みを遮る
		kRenderFlagReflectAll = 1u << 2,			// 映り込みを付けていないマテリアルにも付ける
		kRenderFlagGI = 1u << 3,					// プローブの間接光 (DDGIPass)
		kRenderFlagScreenGI = 1u << 4,				// 画面のプローブの間接光 (ScreenProbeGIPass)
		kRenderFlagScreenReflection = 1u << 5,		// 映り込みを別パスで求める (ScreenReflectionPass)
		kRenderFlagReflectionBounce = 1u << 6,		// 別パスの映り込みで2回目の跳ね返りも出す
		kRenderFlagSpecularAA = 1u << 7,			// ハイライトのちらつきを粗さで抑える
		kRenderFlagReflectionSceneColor = 1u << 8,	// 別パスの映り込みで画面の色を使う (SSR)
		kRenderFlagTranslucentShadow = 1u << 9,		// 半透明の物の色つきの影。TLAS に半透明の物があるフレームだけ立つ
		kRenderFlagLocalLightAO = 1u << 10,			// 影なしのライトにも AO を掛ける
		kRenderFlagScreenSpaceGI = 1u << 11,		// 画面の照り返し (ScreenSpaceGIPass)
		kRenderFlagSoftLocalShadow = 1u << 12,		// 点光源とスポットの柔らかい影 (PunctualShadowPass)
	};

	/// <summary>
	/// 点光源とスポットの柔らかい影の枠 (AllLightsData::softShadowLights)。1バイトに1本ずつ下から並ぶ
	/// </summary>
	/// <remarks>HLSL 側 (Common/ScreenSpaceShadow.hlsli) と同じ値であること</remarks>
	enum SoftShadowSlot : uint32_t {
		kSoftShadowSlotCount = 3,
		kSoftShadowSlotEmpty = 0xFFu,		// 空いている枠
		kSoftShadowSlotSpot = 0x80u,		// 立っていればスポット。下位7ビットが番号
		kSoftShadowSlotsNone = 0xFFFFFFFFu,	// 全部空き
	};

	/// <summary>
	/// 空気(フォグ)。遠くの物ほどこの色へ寄せる
	/// </summary>
	/// <remarks>HLSL 側 (Common/Fog.hlsli) の FogParams と並びが一致していること。設定は配置データの環境が入れる</remarks>
	struct FogData {
		Math::Vector3 color = { 0.6f, 0.7f, 0.8f };	// horizonColor の平均
		float density = 0.0f;			// 濃さ。0で無効
		float heightFalloff = 0.0f;		// 高さによる薄まり方。0なら一様
		float baseHeight = 0.0f;		// 濃さの基準にする高さ
		float startDistance = 0.0f;		// カメラからこの距離までは霞ませない
		float maxOpacity = 1.0f;		// これ以上は霞ませない
		Math::Vector3 cameraPosition{};	// 自前でカメラの定数を持たないシェーダ(空・粒子)用。毎フレーム LightManager が入れる
		float skyAmount = 1.0f;			// 空へどれだけ掛けるか
		// 水平の向きごとの色。+X, -X, +Z, -Z の順。空から色を取らないときは4つとも color と同じにする
		Math::Vector4 horizonColor[4] = { { 0.6f,0.7f,0.8f,0.0f },{ 0.6f,0.7f,0.8f,0.0f },{ 0.6f,0.7f,0.8f,0.0f },{ 0.6f,0.7f,0.8f,0.0f } };
		// 太陽の方向と色。0番の平行光源から毎フレーム LightManager が入れる
		Math::Vector3 sunDirection = { 0.0f,1.0f,0.0f };
		float sunScatter = 0.0f;		// 太陽の方向のフォグを明るくする強さ。0で無効
		Math::Vector3 sunColor = { 1.0f,1.0f,1.0f };
		float sunScatterPower = 8.0f;	// 大きいほど太陽のまわりだけが明るくなる
		uint32_t skyMode = kSkyModeTexture;
		float skyIntensity = 1.0f;		// 計算する空の明るさ
		float cloudCoverage = 0.0f;		// 雲の量。0で雲を出さない
		float cloudTime = 0.0f;			// 雲を流す時刻(秒)
		// 空気遠近。1m あたりの係数を LightManager が毎フレーム入れる
		Math::Vector3 aerialRayleigh{};	// 太陽の光 × 透過率 × レイリー散乱
		float aerialMieG = 0.8f;
		Math::Vector3 aerialMie{};		// 同、ミー散乱
		float aerialDistanceScale = 0.0f;	// 0で空気遠近を掛けない
		Math::Vector3 aerialAmbient{};	// 多重散乱で回り込む光 × 散乱
		float starIntensity = 0.0f;		// 星の明るさ。0で描かない
		Math::Vector3 aerialExtinction{};
		float cloudShadow = 0.0f;		// 雲が平行光源0番を遮る強さ(0〜1)
		Math::Vector3 sunDiskColor{};	// 大気を通った太陽の色
		float moonIntensity = 0.0f;		// 月の明るさ。0で描かない
		Math::Vector3 skyGlowColor{};	// 光害の色 × 明るさ。0で掛からない
		float skyGlowHeight = 0.25f;	// 光害が届く高さ (向きの y)
		// 2層目の霧 (UE の Second Fog Data)。色・始まる距離・上限は1層目と共有し、濃さは1層目に足す
		float secondDensity = 0.0f;		// 濃さ。0で無効
		float secondHeightFalloff = 0.0f;	// 高さによる薄まり方
		float secondBaseHeight = 0.0f;	// 濃さの基準にする高さ (ワールドの高さ)
		// 計算する空で、カメラから見た地面の縁の天頂角。SkyView の数表の並びに使う。毎フレーム LightManager が入れる
		float skyHorizonZenith = 1.5707963f;
	};

	/// <summary>
	/// 全てのライトをまとめた構造体
	/// </summary>
	/// <remarks>HLSL 側の AllLights と並び順・パディングが一致していること</remarks>
	struct AllLightsData {
		DirectionalLight directionalLights[kMaxDirectionalLights];
		PointLightData pointLights[kMaxPointLights];
		SpotLightData spotLights[kMaxSpotLights];
		int32_t numDirectionalLights;
		int32_t numPointLights;
		int32_t numSpotLights;
		// 0番の平行光源の影の中で映り込みをどれだけ暗くするか。0 だと金属の床に影が出ない
		float reflectionShadow = 0.5f;

		// 半球アンビエント。法線の上下で空色と地面色を混ぜる。
		// PBR にすると Half-Lambert の回り込みが無くなるので、これが無いと影側が真っ黒になる
		Math::Vector3 ambientSkyColor = { 0.35f, 0.42f, 0.55f };
		float ambientIntensity = 0.30f;
		Math::Vector3 ambientGroundColor = { 0.20f, 0.17f, 0.14f };

		uint32_t rayTracedShadowMask = kShadowMaskAll;

		// レイトレAO。半球アンビエントにだけ掛かる
		float aoRadius = 2.0f;
		float aoIntensity = 1.0f;
		uint32_t aoSampleCount = 4;
		// デノイザを通すと粒が見えないので既定は Screen(実測 1.18ms)
		uint32_t aoMode = kAOModeScreen;

		// 鏡面の映り込み。絵が変わるので既定は Off。設定は配置データの環境が入れる
		float reflectionMaxDistance = 100.0f;
		float reflectionIntensity = 1.0f;
		uint32_t reflectionMode = kReflectionModeOff;
		// Soft は AO と合わせて約2.0ms。既定は Hard で、要るときだけ ImGui で切り替える
		uint32_t shadowMode = kShadowModeHard;

		// 受け持つのは0番の平行光源だけ。太陽の実際の0.27度では半影が細すぎるので広げてある
		float sunAngularRadius = 0.0175f;	// ラジアン(=約1.0度)
		uint32_t shadowSampleCount = 4;
		// アンビエントの経路。IBL にすると半球近似をやめて焼いたキューブマップを引く。
		// 既定が Hemisphere なのは、切り替えた瞬間に全マテリアルの明るさが変わるため
		uint32_t ambientMode = kAmbientModeHemisphere;
		// IBL のときだけアンビエントに掛ける倍率。環境マップは半球の2色よりずっと明るい
		float iblAmbientScale = 0.6f;

		// 空に掛けている色。映り込みと IBL にも同じ色を掛けて、見えている空と揃える
		Math::Vector3 environmentTint = { 1.0f, 1.0f, 1.0f };
		// 0 なら空を出していない。映り込みは半球の2色で代用する
		uint32_t hasEnvironment = 1;

		uint32_t lightingMode = kLightingModeSimple;
		// RenderFlag の組み合わせ
		uint32_t renderFlags = 0;
		// 柔らかい影の枠 (SoftShadowSlot)。PunctualShadowPass がフレームごとに入れる
		uint32_t softShadowLights = kSoftShadowSlotsNone;
		uint32_t padLighting = 0;

		// 前方描画のPSはここから読む。空と粒子は同じ中身を gFog として別に受け取る
		FogData fog;

		// 濡れた路面。wetness が0なら何もしない
		float wetness = 0.0f;
		float puddleAmount = 0.5f;
		float puddleScale = 4.0f;
		float rainAmount = 0.0f;		// 雨の強さ (0〜1)。波紋を立てる
	};

	// HLSL 側と大きさが食い違うと、ずれた位置から読んで絵が壊れる。並びを変えたらここで気付けるようにしておく
	static_assert(sizeof(FogData) == 272, "FogData の大きさが Fog.hlsli の FogParams と一致していない");
	static_assert(sizeof(AllLightsData) == 1632, "AllLightsData の大きさが Object3d.hlsli の AllLights と一致していない");

	/// <summary>
	/// 計算する空から、フォグ・アンビエント・空気遠近へ何を取るか
	/// </summary>
	struct AtmosphereSettings {
		/// <summary>フォグの色を空の地平線から取る</summary>
		bool fogColorFromSky = false;
		/// <summary>空から取ったフォグの色に掛ける倍率</summary>
		float fogSkyColorScale = 1.0f;
		/// <summary>空気遠近の距離の倍率。0 で掛けない</summary>
		float aerialPerspectiveScale = 1.0f;
		/// <summary>空気遠近を3Dの表から引く。false ならカメラの高さの空気が一様とみなす式</summary>
		bool aerialPerspectiveLut = true;
		/// <summary>空から回り込む光の強さ</summary>
		float skyLightIntensity = 1.0f;
		/// <summary>半球アンビエントに足す明るさ。夜の暗さの下限</summary>
		Math::Vector3 nightAmbient = { 0.0f,0.0f,0.0f };
	};

	/// <summary>
	/// ライト管理クラス
	/// </summary>
	class LightManager {
	public:
		LightManager() = default;
		~LightManager() = default;

	public:

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();
		void Update();

		/// <summary>
		/// ライトの作成（初期化時にデフォルトを作成）
		/// </summary>
		void CreateLight();

		/// <summary>起動時の状態に戻す。平行光源1つ(真下・白・1.0)と、強さ0の点光源1つ</summary>
		/// <remarks>シーンに入るたびに Level が呼ぶ。点光源1つはゲーム側が GetPointLight(0) で使う</remarks>
		void ResetLights();

		/// <summary>
		/// ポイントライトの追加
		/// </summary>
		void AddPointLight();
		/// <returns>入れた番号(GetPointLight に渡す)。上限まで埋まっていたら -1</returns>
		int AddPointLight(const PointLightData& data);

		/// <summary>
		/// スポットライトの追加
		/// </summary>
		void AddSpotLight();
		/// <returns>入れた番号(GetSpotLight に渡す)。上限まで埋まっていたら -1</returns>
		int AddSpotLight(const SpotLightData& data);

		/// <summary>影を落とさないライトの枠を1つ取る。消えた状態(色が0)で入る</summary>
		/// <returns>番号(GetClusteredLight に渡す)。上限まで埋まっていたら -1</returns>
		int AddClusteredLight();
		/// <summary>枠を返す</summary>
		void RemoveClusteredLight(int index);
		ClusteredLightData* GetClusteredLight(int index);
		/// <summary>点いている影なしのライトを詰めて書き出す</summary>
		/// <param name="hasShaped">棒か長方形の光源を書いたか</param>
		/// <returns>書いた数</returns>
		uint32_t GatherClusteredLights(ClusteredLightData* out, uint32_t capacity, bool* hasShaped = nullptr) const;

		/// <summary>
		/// データをコマンドリストに送る
		/// </summary>
		void SetLightCommand(ID3D12GraphicsCommandList* commandList);

		//========================================================================*/
		//* Getter

		/// <summary>ライト定数バッファのGPUアドレス</summary>
		/// <remarks>描画パイプライン以外(AOのCS等)からも同じ設定値を読むために公開している</remarks>
		D3D12_GPU_VIRTUAL_ADDRESS GetLightsGPUVirtualAddress() const;

		/// <summary>フォグだけの定数バッファのGPUアドレス。ライトの構造体を持たないシェーダ(空・粒子)用</summary>
		D3D12_GPU_VIRTUAL_ADDRESS GetFogGPUVirtualAddress() const;

		/// <summary>設定値そのもの。パスを走らせるかどうかの判定に使う</summary>
		const AllLightsData& GetData() const { return allLightsData_; }
		/// <summary>このフレームに GPU へ送った設定。半透明の影のフラグも入っている</summary>
		const AllLightsData& GetUploadedData() const { return uploadedData_; }
		DirectionalLight* GetDirectionLight(int num = 0) { return &allLightsData_.directionalLights[num]; }
		PointLightData* GetPointLight(int num = 0) { return &allLightsData_.pointLights[num]; }
		SpotLightData* GetSpotLight(int num = 0) { return &allLightsData_.spotLights[num]; }

		int GetNumDirectionalLights() const { return allLightsData_.numDirectionalLights; }
		int GetNumPointLights() const { return allLightsData_.numPointLights; }
		int GetNumSpotLights() const { return allLightsData_.numSpotLights; }
		RayTracedAOMode GetAOMode() const { return static_cast<RayTracedAOMode>(allLightsData_.aoMode); }
		RayTracedShadowMode GetShadowMode() const { return static_cast<RayTracedShadowMode>(allLightsData_.shadowMode); }
		/// <summary>RenderFlag の組み合わせ</summary>
		uint32_t GetRenderFlags() const { return allLightsData_.renderFlags; }

		//========================================================================*/
		//* Setter
		void SetNumDirectionalLights(int num) { allLightsData_.numDirectionalLights = num; }
		void SetNumPointLights(int num) { allLightsData_.numPointLights = num; }
		void SetNumSpotLights(int num) { allLightsData_.numSpotLights = num; }
		/// <summary>フォグの設定を入れる。カメラの位置と太陽は毎フレーム自前で入れるので、渡された値は使わない</summary>
		void SetFog(const FogData& fog);
		/// <summary>AO と平行光源の影をどの経路で計算するか。どちらも軽い側なら G-Buffer のプリパスごと止まる</summary>
		void SetRayTracingModes(RayTracedAOMode aoMode, RayTracedShadowMode shadowMode) {
			allLightsData_.aoMode = aoMode;
			allLightsData_.shadowMode = shadowMode;
		}
		/// <summary>アンビエントを半球の2色から取るか、環境マップから焼いたものから取るか</summary>
		/// <param name="iblScale">IBL のときだけアンビエントに掛ける倍率</param>
		void SetAmbientMode(AmbientMode mode, float iblScale) {
			allLightsData_.ambientMode = mode;
			allLightsData_.iblAmbientScale = iblScale;
		}
		void SetLightingMode(LightingMode mode) { allLightsData_.lightingMode = mode; }
		/// <summary>品質の設定で足す描画の細部</summary>
		void SetRenderFlags(uint32_t flags) { allLightsData_.renderFlags = flags; }
		/// <summary>このフレームの定数だけ、半透明の影のフラグを立てる・下ろす</summary>
		void SetTranslucentShadowThisFrame(bool enabled);
		/// <summary>このフレームの定数だけ、柔らかい影の枠を入れる</summary>
		void SetSoftShadowLightsThisFrame(uint32_t softShadowLights);
		/// <summary>濡れた路面の設定を入れる</summary>
		/// <param name="wetness">全体の濡れ具合(0〜1)。0で何もしない</param>
		/// <param name="puddleAmount">水たまりの多さ(0〜1)</param>
		/// <param name="puddleScale">水たまりの大きさ(m)</param>
		/// <summary>雨の強さ (0〜1)</summary>
		void SetRainAmount(float rain) { allLightsData_.rainAmount = rain; }
		void SetWetness(float wetness, float puddleAmount, float puddleScale) {
			allLightsData_.wetness = wetness;
			allLightsData_.puddleAmount = puddleAmount;
			allLightsData_.puddleScale = puddleScale;
		}
		/// <summary>空を計算するクラス。計算する空のとき、フォグ・アンビエント・空気遠近を上書きする</summary>
		void SetSkyAtmosphere(SkyAtmosphere* skyAtmosphere) { skyAtmosphere_ = skyAtmosphere; }
		void SetAtmosphereSettings(const AtmosphereSettings& settings) { atmosphereSettings_ = settings; }
		/// <summary>空・フォグ・空気遠近が使う太陽を、0番の平行光源とは別に入れる</summary>
		/// <param name="toSun">太陽へ向かう単位ベクトル。地平線の下でもよい</param>
		/// <param name="color">太陽の色と強さ</param>
		/// <remarks>夜も空は沈んだ太陽から計算させるために使う。ResetLights で外れる</remarks>
		void SetSkySun(const Math::Vector3& toSun, const Math::Vector3& color) {
			hasSkySun_ = true;
			skySunDirection_ = toSun;
			skySunColor_ = color;
		}
		/// <summary>空の太陽を0番の平行光源に戻す</summary>
		void ClearSkySun() { hasSkySun_ = false; }
		/// <summary>太陽と空から求める値を今の設定から作り直す。Update の後に設定を入れ直したら呼ぶ</summary>
		void RefreshDerived();
		/// <summary>空を計算する設定で、空を出しているか</summary>
		bool IsAtmosphereActive() const {
			return allLightsData_.fog.skyMode == kSkyModeAtmosphere && allLightsData_.hasEnvironment != 0;
		}
		/// <summary>空気遠近を3Dの表から引くか</summary>
		/// <remarks>空から係数がまだ届いていないフレームは false</remarks>
		bool IsAerialPerspectiveLutEnabled() const {
			return atmosphereSettings_.aerialPerspectiveLut && allLightsData_.fog.aerialDistanceScale > 0.0f;
		}
		/// <summary>半球アンビエントの2色を入れる</summary>
		void SetAmbientColors(const Math::Vector3& sky, const Math::Vector3& ground) {
			allLightsData_.ambientSkyColor = sky;
			allLightsData_.ambientGroundColor = ground;
		}
		/// <summary>映り込みと IBL が元にする空の状態を入れる</summary>
		/// <param name="tint">空に掛けている色</param>
		/// <param name="hasSky">空を出しているか</param>
		void SetEnvironmentSky(const Math::Vector3& tint, bool hasSky) {
			allLightsData_.environmentTint = tint;
			allLightsData_.hasEnvironment = hasSky ? 1 : 0;
		}
		/// <summary>鏡面の映り込みの設定を入れる</summary>
		/// <param name="maxDistance">この距離までの物を見る</param>
		/// <param name="shadow">影の中で映り込みをどれだけ暗くするか(0〜1)</param>
		void SetReflection(RayTracedReflectionMode mode, float maxDistance, float intensity, float shadow) {
			allLightsData_.reflectionMode = mode;
			allLightsData_.reflectionMaxDistance = maxDistance;
			allLightsData_.reflectionIntensity = intensity;
			allLightsData_.reflectionShadow = shadow;
		}

		void DebugGUI();

	private:
		void CopyData(uint32_t frameIndex = 0);
		/// <summary>空から読み戻した値を、フォグ・アンビエント・空気遠近へ入れる</summary>
		void ApplyAtmosphere();

	private:

		DXC::DXCom* dxcommon_;
		
		Microsoft::WRL::ComPtr<ID3D12Resource> allLightsResource_[DXC::kFrameCount_];
		AllLightsData* allLightsDataGPU_[DXC::kFrameCount_];
		AllLightsData allLightsData_;
		AllLightsData uploadedData_;

		// allLightsData_.fog の写し。中身は必ず同じ
		Microsoft::WRL::ComPtr<ID3D12Resource> fogResource_[DXC::kFrameCount_];
		FogData* fogDataGPU_[DXC::kFrameCount_] = {};

		SkyAtmosphere* skyAtmosphere_ = nullptr;
		AtmosphereSettings atmosphereSettings_;
		// GPU から最後に届いた値。届くまでは空気遠近を掛けない
		SkyAtmosphere::Summary atmosphereSummary_;
		bool hasAtmosphereSummary_ = false;
		// SetSkySun で入れた太陽。無ければ0番の平行光源を太陽とみなす
		// 影なしのライトの枠。使っていない枠は freeClustered_ に積む
		std::vector<ClusteredLightData> clusteredLights_;
		std::vector<bool> isClusteredUsed_;
		std::vector<int> freeClustered_;
		// SetFog で入れた雲の量。高く昇ると雲を消すので元の値を持つ
		float cloudCoverageBase_ = 0.0f;
		bool hasSkySun_ = false;
		Math::Vector3 skySunDirection_ = { 0.0f,1.0f,0.0f };
		Math::Vector3 skySunColor_ = { 0.0f,0.0f,0.0f };

	};
}