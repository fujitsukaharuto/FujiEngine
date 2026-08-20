#pragma once
#include <Windows.h>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <cassert>
#include <functional>
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
	};

	/// <summary>
	/// 最終出力のトーンマップに送るデータ
	/// </summary>
	struct TonemapParams {
		float exposure = 2.0f;
		TonemapMode mode = TonemapMode::ACES;
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
}
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

		/// <summary>
		/// 特定のポストエフェクトのポップ
		/// </summary>
		/// <param name="effect">種類</param>
		void PopPostEffect(PostEffectList effect);

		//========================================================================*/
		//* RTVForGPUParticle
		void BarrierSetForGPURTV();
		void SynthesisGPURTV();

		//========================================================================*/
		//* Getter
		ID3D12Resource* GetOffscreenResource(uint32_t index) { return offscreenRt_[index].Get(); }
		const D3D12_CLEAR_VALUE& GetClearColorValue() const { return clearColorValue_; }
		const D3D12_CLEAR_VALUE& GetClearColorValueForGPURTV() const { return clearColorValueForGPU_; }

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

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		GrayscaleVertex* vertexData_ = nullptr;

		std::vector<PostEffectPass> postEffects_;
		std::vector<PostEffectPass> validPostEffects_;

		bool isPostEffect_ = true;
		bool isNonePost_ = true;
	};
}