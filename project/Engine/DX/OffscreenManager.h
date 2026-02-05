#pragma once
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <cassert>
#include <wrl.h>

#include "DXCommand.h"
#include "DXCompil.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "PipeKind.h"
#include "TextureManager.h"
#include "Engine/DX/FrameCount.h"


using namespace Microsoft::WRL;

class DXCom;

#pragma region 構造体群
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
/// 衝撃波のデータ
/// </summary>
struct ShockWaveData {
	Math::Vector4 center = { 0.5f,0.5f,0.0f,0.0f };
	float shockTime;
	float radius = 1.0f;
	float intensity = 0.15f;// 歪みの強さ
	float padding;
};

/// <summary>
/// 炎エフェクトのデータ
/// </summary>
struct FireElement {
	float animeTime; // アニメーション時間
	Math::Vector2 resolution; // 画面解像度
	float distortionStrength = 0.18f; // UVディストーションの強度
	float highlightStrength = 0.6f; // ハイライトの強度
	float detailScale = 7.99f; // 細かいノイズのスケール
	Math::Vector2 rangeMin = { 0.05f,0.3f };  // 炎の描画範囲（最小UV）
	Math::Vector2 rangeMax = { 0.93f,2.82f }; // 炎の描画範囲（最大UV）
	float scale = 1.20f; // Voronoiノイズのスケール
	float speed = 4.01f; // 炎の揺らぎ速度
	float noiseSpeed = -0.12f; // 細かいノイズの移動速度
	float blendStrength = 2.0f;// どれくらい混ぜるか
};

/// <summary>
/// CRTエフェクトのデータ
/// </summary>
struct CRTElemnt {
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
/// 雷エフェクトのデータ
/// </summary>
struct LightningElement {
	Math::Vector2 startPos = { 0.5f,0.3f };
	Math::Vector2 endPos = { 0.5f,0.8f };
	Math::Vector2 rangeMin = { 0.0f,0.0f }; // 描画範囲（最小UV）
	Math::Vector2 rangeMax = { 1.0f,1.0f }; // 描画範囲（最大UV）
	Math::Vector2 resolution; // 画面解像度
	float time; // アニメーション時間
	float mainBranchStrength = 25.0f; // 主幹の強度
	float branchCount = 4.0f; // 分岐の数
	float branchFade = 20.0f; // 分岐のフェード率
	float highlightStrength = 15.0f; // ハイライトの強度
	float noiseScale = 0.2f; // ノイズのスケール
	float noiseSpeed = 5.0f; // ノイズの移動速度
	float branchStrength = 4.0f;
	float boltCount = 3.0f;
	float progress;
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
#pragma endregion

namespace Graphics {
	/// <summary>
	/// オフスクリーン管理クラス
	/// </summary>
	class OffscreenManager {
	public:
		OffscreenManager() = default;
		~OffscreenManager();

	public:

		void Initialize(DXCom* dxcom);
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

		/// <summary>
		/// 特定のポストエフェクトのポップ
		/// </summary>
		/// <param name="effect">種類</param>
		void PopPostEffect(PostEffectList effect);

		//========================================================================*/
		//* Getter
		ID3D12Resource* GetOffscreenResource(uint32_t index) { return offscreenRt_[index].Get(); }
		const D3D12_CLEAR_VALUE& GetClearColorValue() const { return clearColorValue_; }

		//========================================================================*/
		//* Setter
		void SetRadialParamsCenter(const Math::Vector2& center) { radialData_.center = center; }
		void SetRadialParamsWidth(float width) { radialData_.blurWidth = width; }

	private:

		/// <summary>
		/// 頂点情報の初期化
		/// </summary>
		void SettingVertex();

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

		/// <summary>
		/// データを現在のフレームのリソースにコピー
		/// </summary>
		/// <param name="frameIndex">フレーム番号</param>
		void CopyData(uint32_t frameIndex = 0);

	private:

		DXCom* dxcommon_ = nullptr;

		ComPtr<ID3D12Resource> offscreenRt_[DXC::kFrameCount_];
		D3D12_RENDER_TARGET_VIEW_DESC offscreenRTVDesc_{};
		D3D12_CLEAR_VALUE clearColorValue_{};
		uint32_t offscreenSRVIndex_[DXC::kFrameCount_];
		uint32_t offscreenIndex_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE offTextureHandle_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE offTextureHandleCPU_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE offTextureUAVHandleCPU_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE offTextureUAVHandle_[DXC::kFrameCount_];


		ComPtr<ID3D12Resource> grayCSResource_[DXC::kFrameCount_];
		GrayCS* grayCSDataGPU_[DXC::kFrameCount_];
		GrayCS grayCSData_;

		ComPtr<ID3D12Resource> shockResource_[DXC::kFrameCount_];
		ShockWaveData* shockDataGPU_[DXC::kFrameCount_];
		ShockWaveData shockData_;

		ComPtr<ID3D12Resource> fireResource_[DXC::kFrameCount_];
		FireElement* fireDataGPU_[DXC::kFrameCount_];
		FireElement fireData_;

		ComPtr<ID3D12Resource> thunderResource_[DXC::kFrameCount_];
		LightningElement* thunderDataGPU_[DXC::kFrameCount_];
		LightningElement thunderData_;
		int nowTex_;

		ComPtr<ID3D12Resource> cRTResource_[DXC::kFrameCount_];
		CRTElemnt* crtDataGPU_[DXC::kFrameCount_];
		CRTElemnt crtData_;

		ComPtr<ID3D12Resource> outlineResource_[DXC::kFrameCount_];
		OutlineElement* outlineDataGPU_[DXC::kFrameCount_];
		OutlineElement outlineData_;

		ComPtr<ID3D12Resource> bloomResource_[DXC::kFrameCount_];
		BloomParams* bloomDataGPU_[DXC::kFrameCount_];
		BloomParams bloomData_;

		ComPtr<ID3D12Resource> radialResource_[DXC::kFrameCount_];
		RadialParams* radialDataGPU_[DXC::kFrameCount_];
		RadialParams radialData_;

		ComPtr<ID3D12Resource> vignetteResource_[DXC::kFrameCount_];
		VignetteData* vignetteDataGPU_[DXC::kFrameCount_];
		VignetteData vignetteData_;

		Texture* baseTex_;
		Texture* vNoiseTex_;
		Texture* noiseTex_;
		Texture* noiseDirTex_;


		ComPtr<ID3D12Resource> vertexGrayResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexGrayBufferView_{};
		GrayscaleVertex* grayVertexDate_ = nullptr;


		ComPtr<ID3D12Resource> outputTexture_[DXC::kFrameCount_];
		uint32_t outputIndex_[DXC::kFrameCount_];
		uint32_t outputSRVIndex_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE outputSRVHandleCPU_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE outputSRVHandle_[DXC::kFrameCount_];
		D3D12_CPU_DESCRIPTOR_HANDLE outputUAVHandleCPU_[DXC::kFrameCount_];
		D3D12_GPU_DESCRIPTOR_HANDLE outputUAVHandle_[DXC::kFrameCount_];

		std::vector<PostEffectPass> postEffects_;
		std::vector<PostEffectPass> validPostEffects_;

		bool isGrayscale_ = true;
		bool isNonePost_ = true;
		bool isShockWave_ = true;
		bool isFire_ = true;
		bool isThunder_ = true;

		float maxThunderTime_ = 1.5f;
	};
}