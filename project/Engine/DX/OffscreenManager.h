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
	Math::Vector3 gray_;
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
	Math::Vector4 center;
	float shockTime;
	float radius;
	float intensity;// 歪みの強さ
	float padding;
};

/// <summary>
/// 炎エフェクトのデータ
/// </summary>
struct FireElement {
	float animeTime; // アニメーション時間
	Math::Vector2 resolution; // 画面解像度
	float distortionStrength; // UVディストーションの強度
	float highlightStrength; // ハイライトの強度
	float detailScale; // 細かいノイズのスケール
	Math::Vector2 rangeMin; // 炎の描画範囲（最小UV）
	Math::Vector2 rangeMax; // 炎の描画範囲（最大UV）
	float scale; // Voronoiノイズのスケール
	float speed; // 炎の揺らぎ速度
	float noiseSpeed; // 細かいノイズの移動速度
	float blendStrength;// どれくらい混ぜるか
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
	float bloomThreshold; // しきい値（例：1.0）
	float bloomIntensity; // ブルーム強度（例：1.2）
};

/// <summary>
/// ラジアルブラーのデータ
/// </summary>
struct RadialParams {
	Math::Vector2 center;
	float blurWidth;
};

/// <summary>
/// 雷エフェクトのデータ
/// </summary>
struct LightningElement {
	Math::Vector2 startPos;
	Math::Vector2 endPos;
	Math::Vector2 rangeMin; // 描画範囲（最小UV）
	Math::Vector2 rangeMax; // 描画範囲（最大UV）
	Math::Vector2 resolution; // 画面解像度
	float time; // アニメーション時間
	float mainBranchStrength; // 主幹の強度
	float branchCount; // 分岐の数
	float branchFade; // 分岐のフェード率
	float highlightStrength; // ハイライトの強度
	float noiseScale; // ノイズのスケール
	float noiseSpeed; // ノイズの移動速度
	float branchStrngth;
	float boltCount;
	float progres;
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

		void CreateResource();
		void SettingTexture();
		void Command();

		/// <summary>
		/// ポストエフェクトのリセット
		/// </summary>
		void ResetPostEffect() { validPostEffects_.clear(); }

		/// <summary>
		/// ポストエフェクトの追加
		/// </summary>
		void AddPostEffect(PostEffectList effect) { validPostEffects_.push_back(postEffects_[int(effect)]); }

		/// <summary>
		/// 特定のポストエフェクトのポップ
		/// </summary>
		void PopPostEffect(PostEffectList effect);

		//========================================================================*/
		//* Getter
		ID3D12Resource* GetOffscreenResource(uint32_t index) { return offscreenrt_[index].Get(); }
		const D3D12_CLEAR_VALUE& GetClearColorValue() const { return clearColorValue_; }

		//========================================================================*/
		//* Setter
		void SetRadialParamsCenter(const Math::Vector2& center) { radialData_.center = center; }
		void SetRadialParamsWidth(float width) { radialData_.blurWidth = width; }

	private:

		void SettingVertex();
		void InitializePostEffects();
		void CopyData(uint32_t frameIndex = 0);

	private:

		DXCom* dxcommon_ = nullptr;

		ComPtr<ID3D12Resource> offscreenrt_[DXC::kFrameCount_];
		D3D12_RENDER_TARGET_VIEW_DESC offscreenrtvDesc_{};
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
		Texture* voronoTex_;
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

	};
}