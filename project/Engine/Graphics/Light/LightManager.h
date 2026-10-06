#pragma once
#include "Engine/Graphics/Light/DirectionLight.h"
#include "Engine/Graphics/Light/PointLight.h"
#include "Engine/Graphics/Light/SpotLight.h"
#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl.h>

namespace DXC { class DXCom; }

namespace Graphics {

	const int kMaxDirectionalLights = 3;
	const int kMaxPointLights = 10;
	const int kMaxSpotLights = 10;

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
		float pad0;

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
		uint32_t pad3 = 0;

		// 前方描画のPSはここから読む。空と粒子は同じ中身を gFog として別に受け取る
		FogData fog;
	};

	// HLSL 側と大きさが食い違うと、ずれた位置から読んで絵が壊れる。並びを変えたらここで気付けるようにしておく
	static_assert(sizeof(FogData) == 112, "FogData の大きさが Fog.hlsli の FogParams と一致していない");
	static_assert(sizeof(AllLightsData) == 1424, "AllLightsData の大きさが Object3d.hlsli の AllLights と一致していない");

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
		/// <remarks>シーンに入るたびに Level が呼ぶ。点光源1つはゲーム側が GetPointLight(0) を使う前提のため</remarks>
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
		DirectionalLight* GetDirectionLight(int num = 0) { return &allLightsData_.directionalLights[num]; }
		PointLightData* GetPointLight(int num = 0) { return &allLightsData_.pointLights[num]; }
		SpotLightData* GetSpotLight(int num = 0) { return &allLightsData_.spotLights[num]; }

		int GetNumDirectionalLights() const { return allLightsData_.numDirectionalLights; }
		int GetNumPointLights() const { return allLightsData_.numPointLights; }
		int GetNumSpotLights() const { return allLightsData_.numSpotLights; }

		//========================================================================*/
		//* Setter
		void SetNumDirectionalLights(int num) { allLightsData_.numDirectionalLights = num; }
		void SetNumPointLights(int num) { allLightsData_.numPointLights = num; }
		void SetNumSpotLights(int num) { allLightsData_.numSpotLights = num; }
		/// <summary>フォグの設定を入れる。カメラの位置は毎フレーム自前で入れるので、渡された値は使わない</summary>
		void SetFog(const FogData& fog);
		/// <summary>鏡面の映り込みの設定を入れる</summary>
		/// <param name="maxDistance">この距離までの物を見る</param>
		void SetReflection(RayTracedReflectionMode mode, float maxDistance, float intensity) {
			allLightsData_.reflectionMode = mode;
			allLightsData_.reflectionMaxDistance = maxDistance;
			allLightsData_.reflectionIntensity = intensity;
		}

		void DebugGUI();

	private:
		void CopyData(uint32_t frameIndex = 0);

	private:

		DXC::DXCom* dxcommon_;
		
		Microsoft::WRL::ComPtr<ID3D12Resource> allLightsResource_[DXC::kFrameCount_];
		AllLightsData* allLightsDataGPU_[DXC::kFrameCount_];
		AllLightsData allLightsData_;

		// allLightsData_.fog の写し。中身は必ず同じ
		Microsoft::WRL::ComPtr<ID3D12Resource> fogResource_[DXC::kFrameCount_];
		FogData* fogDataGPU_[DXC::kFrameCount_] = {};

	};
}