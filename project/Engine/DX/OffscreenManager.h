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


using namespace Microsoft::WRL;

class DXCom;

#pragma region 構造体群
struct GrayCS {
	Vector3 gray_;
};

struct VignetteData {
	Vector3 color_;
};

struct GrayscaleVertex {
	Vector4 position;
	Vector2 texcoord;
};

struct ShockWaveData {
	Vector4 center;
	float shockTime;
	float radius;
	float intensity;// 歪みの強さ
	float padding;
};

struct FireElement {
	float animeTime; // アニメーション時間
	Vector2 resolution; // 画面解像度
	float distortionStrength; // UVディストーションの強度
	float highlightStrength; // ハイライトの強度
	float detailScale; // 細かいノイズのスケール
	Vector2 rangeMin; // 炎の描画範囲（最小UV）
	Vector2 rangeMax; // 炎の描画範囲（最大UV）
	float scale; // Voronoiノイズのスケール
	float speed; // 炎の揺らぎ速度
	float noiseSpeed; // 細かいノイズの移動速度
	float blendStrength;// どれくらい混ぜるか
};

struct CRTElemnt {
	float crtTime;
	Vector2 resolution;
};

struct OutlineElement {
	Matrix4x4 projectionInverse;
};

struct BloomParams {
	float bloomThreshold; // しきい値（例：1.0）
	float bloomIntensity; // ブルーム強度（例：1.2）
};

struct RadialParams {
	Vector2 center;
	float blurWidth;
};

struct LightningElement {
	Vector2 startPos;
	Vector2 endPos;
	Vector2 rangeMin; // 描画範囲（最小UV）
	Vector2 rangeMax; // 描画範囲（最大UV）
	Vector2 resolution; // 画面解像度
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

struct PostEffectPass {
	Pipe pipeline; // 使用するパイプライン名
	PostEffectList effectName;

	// SRV/UAV/CBVのセット処理
	std::function<void(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE input, D3D12_GPU_DESCRIPTOR_HANDLE output)> setup;
};
#pragma endregion

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


	ComPtr<ID3D12Resource>& GetOffscreenResource() { return offscreenrt_; }
	const D3D12_CLEAR_VALUE& GetClearColorValue() const { return clearColorValue_; }

	void ResetPostEffect() { validPostEffects.clear(); }
	void AddPostEffect(PostEffectList effect) { validPostEffects.push_back(postEffects[int(effect)]); }
	void PopPostEffect(PostEffectList effect);

	// Radial
	void SetRadialParamsCenter(Vector2 center) { radialData_->center = center; }
	void SetRadialParamsWidth(float width) { radialData_->blurWidth = width; }

private:

	void SettingVertex();
	void InitializePostEffects();

private:

	DXCom* dxcommon_ = nullptr;

	ComPtr<ID3D12Resource> offscreenrt_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC offscreenrtvDesc_{};
	D3D12_CLEAR_VALUE clearColorValue_{};
	uint32_t offscreenSRVIndex_;
	uint32_t offscreenIndex_;
	D3D12_GPU_DESCRIPTOR_HANDLE offTextureHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE offTextureHandleCPU_;
	D3D12_CPU_DESCRIPTOR_HANDLE offTextureUAVHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE offTextureUAVHandle_;


	ComPtr<ID3D12Resource> grayCSResource_ = nullptr;
	GrayCS* grayCSData_;

	ComPtr<ID3D12Resource> shockResource_ = nullptr;
	ShockWaveData* shockData_;

	ComPtr<ID3D12Resource> fireResource_ = nullptr;
	FireElement* fireData_;

	ComPtr<ID3D12Resource> thunderResource_ = nullptr;
	LightningElement* thunderData_;
	int nowTex;

	ComPtr<ID3D12Resource> cRTResource_ = nullptr;
	CRTElemnt* crtData_;

	ComPtr<ID3D12Resource> outlineResource_ = nullptr;
	OutlineElement* outlineData_;

	ComPtr<ID3D12Resource> bloomResource_ = nullptr;
	BloomParams* bloomData_;

	ComPtr<ID3D12Resource> radialResource_ = nullptr;
	RadialParams* radialData_;

	ComPtr<ID3D12Resource> vignetteResource_ = nullptr;
	VignetteData* vignetteData_;

	Texture* baseTex_;
	Texture* voronoTex_;
	Texture* noiseTex_;
	Texture* noiseDirTex_;


	ComPtr<ID3D12Resource> vertexGrayResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexGrayBufferView_{};
	GrayscaleVertex* grayVertexDate_ = nullptr;
	

	ComPtr<ID3D12Resource> outputTexture_;
	uint32_t outputIndex_;
	uint32_t outputSRVIndex_;
	D3D12_CPU_DESCRIPTOR_HANDLE outputSRVHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE outputSRVHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE outputUAVHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAVHandle_;

	std::vector<PostEffectPass> postEffects;
	std::vector<PostEffectPass> validPostEffects;

	bool isGrayscale_ = true;
	bool isNonePost_ = true;
	bool isShockWave_ = true;
	bool isFire_ = true;
	bool isThunder_ = true;

};
