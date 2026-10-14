#pragma once
#include <Windows.h>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <cassert>
#include <functional>
#include <memory>
#include <wrl.h>

#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/Shader/DXCompile.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/DXC/FrameCount.h"


namespace DXC { class DXCom; }

#pragma region 構造体群
namespace Graphics {
	/// <summary>
	/// グレースケールの色
	/// </summary>
	struct GrayCS {
		Math::Vector3 gray_= { 0.2f,0.4f,0.2f };
	};

	/// <summary>
	/// ヴィネットの色
	/// </summary>
	struct VignetteData {
		Math::Vector3 color_;
	};

	/// <summary>
	/// グレースケールのポストエフェクトTextureのvertexData
	/// </summary>
	struct GrayscaleVertex {
		Math::Vector4 position;
		Math::Vector2 texcoord;
	};

	/// <summary>
	/// CRTエフェクトのデータ
	/// </summary>
	struct CRTElement {
		float crtTime;
		Math::Vector2 resolution;
	};

	/// <summary>
	/// OutLineのポストエフェクト時に送るデータ
	/// </summary>
	struct OutlineElement {
		Math::Matrix4x4 projectionInverse;
	};

	/// <summary>
	/// ブルームのデータ
	/// </summary>
	struct BloomParams {
		float bloomThreshold = 0.75f; // しきい値（例：1.0）
		float bloomIntensity = 1.0f; // ブルーム強度（例：1.2）
	};

	/// <summary>
	/// ラジアルブラーのデータ
	/// </summary>
	struct RadialParams {
		Math::Vector2 center = { 0.5f, 0.5f };
		float blurWidth = 0.01f;
	};

	/// <summary>
	/// トーンマップの演算子
	/// </summary>
	/// <remarks>Tonemap.PS.hlsl の switch と数値を一致させること</remarks>
	enum class TonemapMode : uint32_t {
		None,		// 素通し。HDR化の切り分け用
		Reinhard,
		ACES,
		AgX,		// 明るい原色が別の色へ転びにくい。素のままだとコントラストは低め
	};

	/// <summary>
	/// 最終出力のトーンマップに送るデータ
	/// </summary>
	/// <remarks>Tonemap.PS.hlsl の TonemapParams と並びが一致していること</remarks>
	struct TonemapParams {
		float exposure = 2.0f;
		TonemapMode mode = TonemapMode::ACES;
		uint32_t enableFXAA = 1;	// 輪郭のギザギザをならす
		uint32_t enableDither = 1;	// 8bitへ落とすときの縞を散らす
		float saturation = 1.0f;	// 彩度。1でそのまま
		float contrast = 1.0f;		// コントラスト。1でそのまま
		float sharpen = 0.0f;		// 輪郭をくっきりさせる強さ。0で何もしない
		float vignette = 0.0f;		// 画面の四隅を暗くする強さ。0で何もしない
		Math::Vector3 shadowTint = { 1.0f,1.0f,1.0f };		// 暗い所に掛ける色。白でそのまま
		float pad0 = 0.0f;
		Math::Vector3 highlightTint = { 1.0f,1.0f,1.0f };	// 明るい所に掛ける色。白でそのまま
		float pad1 = 0.0f;
	};

	/// <summary>
	/// 光の筋(ゴッドレイ)に送るデータ
	/// </summary>
	/// <remarks>GodRay.CS.hlsl の GodRayParams と並びが一致していること</remarks>
	struct GodRayParams {
		Math::Vector2 sunUV = { 0.5f,0.5f };	// 太陽の画面上の位置。画面の外のこともある
		float intensity = 0.0f;					// 0以下なら何もしない
		float rayLength = 0.6f;					// 太陽までの道のりのうち、どこまで辿るか
		Math::Vector3 color = { 1.0f,1.0f,1.0f };
		float sunSize = 0.4f;					// 光源とみなす範囲の半径。画面の高さが1
	};

	/// <summary>
	/// 被写界深度に送るデータ
	/// </summary>
	/// <remarks>DepthOfField.CS.hlsl の DepthOfFieldParams と並びが一致していること</remarks>
	struct DepthOfFieldParams {
		float focusDistance = 10.0f;	// ピントの合う距離
		float focusRange = 5.0f;		// ピントの前後、この幅まではぼかさない
		float falloff = 20.0f;			// そこからこの距離をかけて最大のぼけに達する
		float maxBlur = 0.0f;			// ぼけの最大半径(画素)。0以下なら何もしない
		Math::Vector2 depthToView = { 0.0f,1.0f };	// 深度 d からカメラ空間の距離へ: y / (d - x)
		Math::Vector2 pad = { 0.0f,0.0f };
	};

	/// <summary>
	/// シーンごとに決める画面の仕上げ。配置データの環境(Level の environment)が持ち、シーンに入るたびに適用する
	/// </summary>
	struct PostProcessSettings {
		bool bloomEnabled = true;
		float bloomThreshold = 0.75f;	// これより明るい所が光る
		float bloomIntensity = 1.0f;
		float bloomKnee = 0.1f;			// しきい値の前後をなだらかにする幅
		float bloomScatter = 0.7f;		// 光の広がり
		TonemapMode tonemapMode = TonemapMode::ACES;
		float exposure = 2.0f;
		float saturation = 1.0f;
		float contrast = 1.0f;
		float sharpen = 0.0f;			// 輪郭をくっきりさせる強さ
		float vignette = 0.0f;			// 四隅を暗くする強さ。被ダメージの赤いビネットとは別物
		Math::Vector3 shadowTint = { 1.0f,1.0f,1.0f };		// 暗い所に掛ける色
		Math::Vector3 highlightTint = { 1.0f,1.0f,1.0f };	// 明るい所に掛ける色
		float godRayIntensity = 0.0f;	// 光の筋の強さ。0で掛からない。色と向きは平行光源から取る
		float godRayLength = 0.6f;		// 筋の長さ。1で太陽まで届く
		float godRaySunSize = 0.4f;		// 光源とみなす太陽のまわりの広さ
		float dofBlur = 0.0f;			// 被写界深度のぼけの最大半径(画素)。0で掛からない
		float dofFocusDistance = 10.0f;	// ピントの合う距離
		float dofFocusRange = 5.0f;		// ピントの前後、この幅まではぼかさない
		float dofFalloff = 20.0f;		// そこからこの距離をかけて最大のぼけに達する
	};

	enum class PostEffectList : int {
		Gray,
		CRT,
		RetroTV,
		Gauss,
		BoxFilter,
		Radial,
		Vignette,
		Outline,
		LuminanceOutline,
		Bloom,
		Random,
		GodRay,
		DepthOfField,
	};

	/// <summary>
	/// ポストエフェクトの1パス分の情報を管理する構造体
	/// </summary>
	struct PostEffectPass {
		Pipe pipeline; // 使用するパイプライン名
		PostEffectList effectName;

		// SRV/UAV/CBVのセット処理
		std::function<void(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE input, D3D12_GPU_DESCRIPTOR_HANDLE output)> setup;
	};
}
#pragma endregion

namespace Graphics {
	class BloomPass;

	/// <summary>
	/// オフスクリーン管理クラス
	/// </summary>
	class OffscreenManager {
	public:
		// bloomPass_ が不完全型なので、コンストラクタもデストラクタも .cpp 側に置く
		OffscreenManager();
		~OffscreenManager();

	public:

		void Initialize(DXC::DXCom* dxcom);
		void Update();
		void DebugGUI();
		void EffectListGUI();

		/// <summary>
		/// リソースの作成
		/// </summary>
		void CreateResource();

		/// <summary>
		/// テクスチャ等の必要リソースの作成
		/// </summary>
		void SettingTexture();

		/// <summary>
		/// コマンド、命令の発行
		/// </summary>
		void Command();

		/// <summary>
		/// ポストエフェクトのリセット
		/// </summary>
		void ResetPostEffect() { validPostEffects_.clear(); }

		/// <summary>
		/// ポストエフェクトの追加
		/// </summary>
		/// <param name="effect">種類</param>
		void AddPostEffect(PostEffectList effect) { validPostEffects_.push_back(postEffects_[int(effect)]); }

		bool HasPostEffect(PostEffectList effect) const;

		/// <summary>
		/// 特定のポストエフェクトのポップ
		/// </summary>
		/// <param name="effect">種類</param>
		void PopPostEffect(PostEffectList effect);

		/// <summary>ブルームとトーンマップの設定を入れる</summary>
		/// <remarks>ブルーム・光の筋・被写界深度以外の効果(ラジアルブラー等、ゲームが途中で足すもの)には触らない</remarks>
		void ApplySettings(const PostProcessSettings& settings);

		/// <summary>次に描くゲーム画面を generated/screenshots/ へ PNG で保存する</summary>
		/// <remarks>_DEBUGMODE でだけ働く。保存は GPU が描き終わる次のフレーム</remarks>
		void RequestScreenshot() { isScreenshotRequested_ = true; }

		//========================================================================*/
		//* RTVForGPUParticle
		void BarrierSetForGPURTV();
		void SynthesisGPURTV();

		//========================================================================*/
		//* Getter
		ID3D12Resource* GetOffscreenResource(uint32_t index) { return offscreenRt_[index].Get(); }
		const D3D12_CLEAR_VALUE& GetClearColorValue() const { return clearColorValue_; }
		const D3D12_CLEAR_VALUE& GetClearColorValueForGPURTV() const { return clearColorValueForGPU_; }
		/// <summary>デバッグGUIのパネルに貼るゲーム画面。_DEBUGMODE でしか作らない</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGameViewTexture() const;

		//========================================================================*/
		//* Setter
		void SetRadialParamsCenter(const Math::Vector2& center) { radialData_.center = center; }
		void SetRadialParamsWidth(float width) { radialData_.blurWidth = width; }

	private:

		/// <summary>
		/// 頂点情報の初期化
		/// </summary>
		void SettingVertex();

		/// <summary>今のカメラと平行光源から、光の筋の太陽の位置と強さを決める</summary>
		/// <remarks>太陽が背後へ回ると強さが0になり、シェーダは素通しになる</remarks>
		void UpdateGodRay();

		/// <summary>描き終えたゲーム画面を、CPU から読めるバッファへ写す</summary>
		/// <param name="isOnPanel">false ならパネル用のテクスチャに絵が無いので、撮るためにもう一度描く</param>
		void CaptureScreenshot(bool isOnPanel);
		void SaveScreenshot();

		/// <summary>
		/// フレームごとのデータリソースの作成
		/// </summary>
		void InitDataResource();

		/// <summary>
		/// 必要データの作成
		/// </summary>
		void InitData();

		/// <summary>
		/// ポストエフェクトの1パス分の情報の作成
		/// </summary>
		void InitializePostEffects();

		/// <summary>
		/// ポストエフェクトの命令発行をPingPong形式で
		/// </summary>
		void PingPongCommand();

		/// <summary>
		/// 別パスでの描画
		/// </summary>
		void OtherPipeLineCommand();

		/// <summary>トーンマップして画面へ出す。パネルに映すときはパネル用のテクスチャへ描く</summary>
		void DrawToScreen(D3D12_GPU_DESCRIPTOR_HANDLE input);

		/// <summary>
		/// データを現在のフレームのリソースにコピー
		/// </summary>
		/// <param name="frameIndex">フレーム番号</param>
		void CopyData(uint32_t frameIndex = 0);

	private:

		DXC::DXCom* dxcommon_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> offscreenRt_[DXC::kFrameCount_];
		D3D12_RENDER_TARGET_VIEW_DESC offscreenRTVDesc_{};
		D3D12_CLEAR_VALUE clearColorValue_{};
		uint32_t offscreenSRVIndex_[DXC::kFrameCount_];
		uint32_t offscreenIndex_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE offTextureHandle_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE offTextureHandleCPU_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE offTextureUAVHandleCPU_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE offTextureUAVHandle_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> outputTexture_[DXC::kFrameCount_];
		uint32_t outputIndex_[DXC::kFrameCount_];
		uint32_t outputSRVIndex_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE outputSRVHandleCPU_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE outputSRVHandle_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE outputUAVHandleCPU_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE outputUAVHandle_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> gpuParticleRt_[DXC::kFrameCount_];
		D3D12_CLEAR_VALUE clearColorValueForGPU_{};
		uint32_t gpuParticleSRVIndex_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE gpuParticleHandle_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE gpuParticleHandleCPU_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> gameViewRt_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> gameViewRtvHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE gameViewRtvHandle_[DXC::kFrameCount_]{};
		D3D12_GPU_DESCRIPTOR_HANDLE gameViewSrvHandle_[DXC::kFrameCount_]{};

		Microsoft::WRL::ComPtr<ID3D12Resource> grayCSResource_[DXC::kFrameCount_];
		GrayCS* grayCSDataGPU_[DXC::kFrameCount_];
		GrayCS grayCSData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> cRTResource_[DXC::kFrameCount_];
		CRTElement* crtDataGPU_[DXC::kFrameCount_];
		CRTElement crtData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> outlineResource_[DXC::kFrameCount_];
		OutlineElement* outlineDataGPU_[DXC::kFrameCount_];
		OutlineElement outlineData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> bloomResource_[DXC::kFrameCount_];
		BloomParams* bloomDataGPU_[DXC::kFrameCount_];
		BloomParams bloomData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> radialResource_[DXC::kFrameCount_];
		RadialParams* radialDataGPU_[DXC::kFrameCount_];
		RadialParams radialData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vignetteResource_[DXC::kFrameCount_];
		VignetteData* vignetteDataGPU_[DXC::kFrameCount_];
		VignetteData vignetteData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> tonemapResource_[DXC::kFrameCount_];
		TonemapParams* tonemapDataGPU_[DXC::kFrameCount_];
		TonemapParams tonemapData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> godRayResource_[DXC::kFrameCount_];
		GodRayParams* godRayDataGPU_[DXC::kFrameCount_];
		GodRayParams godRayData_;
		// 設定された強さ。godRayData_.intensity は太陽の位置に応じて毎フレームここから決める
		float godRayIntensity_ = 0.0f;

		Microsoft::WRL::ComPtr<ID3D12Resource> dofResource_[DXC::kFrameCount_];
		DepthOfFieldParams* dofDataGPU_[DXC::kFrameCount_];
		DepthOfFieldParams dofData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		GrayscaleVertex* vertexData_ = nullptr;

		std::vector<PostEffectPass> postEffects_;
		std::vector<PostEffectPass> validPostEffects_;

		// 多段のブルーム。Bloom の番が来たら、1回のディスパッチの代わりにこれを通す
		std::unique_ptr<BloomPass> bloomPass_;
		// false にすると従来の 5x5 1回のブルームへ戻る(見比べ用)
		bool useMultiScaleBloom_ = true;

		bool isScreenshotRequested_ = false;
		// GPU へ写しを頼んだ。次のフレームに読んで保存する
		bool isScreenshotPending_ = false;
		Microsoft::WRL::ComPtr<ID3D12Resource> screenshotReadback_;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT screenshotFootprint_{};

		bool isPostEffect_ = true;
		bool isNonePost_ = true;
	};
}