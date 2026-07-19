#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <map>
#include "../Particle.h"
#include "GPUEmitter/SphereEmitter.h"
#include "GPUEmitter/TextureBasedEmitter.h"
#include "GPUEmitter/MeshSurfaceEmitter.h"

#include "Model.h"
#include "Object3d.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"
#include "Engine/DX/GPUTimer.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクル用行列データ
/// </summary>
struct PerView {
	Math::Matrix4x4 viewProjection;
	Math::Matrix4x4 billboardMatrix;
};

/// <summary>
/// GPUパーティクル用フレームデータ (256バイトアライメント)
/// </summary>
struct PerFrame {
	float time;
	float deltaTime;
	uint32_t yOffset;
	float padding1;
	Math::Vector4 frustumPlanes[6]; // 視錐台の6平面 (ax + by + cz + d = 0)
	uint32_t padding2[36]; // 256バイト境界に合わせる
};

struct DrawIndexedArgs {
	uint32_t IndexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartIndexLocation;
	int32_t  BaseVertexLocation;
	uint32_t StartInstanceLocation;
};

/// <summary>
/// コンピュート・スプラット用パラメータ (CB)
/// </summary>
struct SplatParamCB {
	uint32_t dimsX;
	uint32_t dimsY;
	uint32_t enableDepthTest; // 1=シーン遮蔽の深度テスト有効(フル解像度時のみ)
	uint32_t pad1;
};

/// <summary>
/// 加速場
/// </summary>
struct AcceleFiled {
	Math::Vector3 Accele;
	Math::AABB area;
};

enum class PipelinePhase { Texture, Surface, Sphere };

struct EmitterInfo {
	std::unique_ptr<IGPUEmitter> emitter;
	PipelinePhase phase;
};

class DXCom;
class SRVManager;

namespace Editor {
	class GPUParticleSystemEditor;
}

struct ParticleCSInstance {
	ComPtr<ID3D12Resource> particleCSInstancing_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSSRVHandle_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSUAVHandle_;
};

/// <summary>
/// GPUパーティクル管理クラス
/// </summary>
class GPUParticleSystem {
#ifdef _DEBUGMODE
	friend class Editor::GPUParticleSystemEditor;
#endif // _DEBUGMODE
public:
	GPUParticleSystem();
	~GPUParticleSystem();

public:

	enum TimerID {
		kTimer_DrawExecuteIndirect = 0,
	};

	enum ComputeTimerID {
		kTimer_ParticleUpdate = 0,
		kTimer_EmitterDispatch = 1,
		kTimer_AliveCountDispatch = 2,
	};

	void Initialize(DXCom* pDxcom, SRVManager* srvManager);
	void Finalize();
	void Update(const Math::Matrix4x4& billboardMatrix);
	void Dispatch();
	void Draw(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView);
	void ResetEmitters();
	void InitDefaultEmitter();

	int InitGPUEmitter(int returnMod = 0);
	int InitGPUEmitterTexture(const std::string& fileName = "white2x2.png");
	int InitGPUEmitterSurface(const std::string& fileName);

	/// <summary>タブ形式のエミッター編集ウィンドウ(GPUParticleScene用)</summary>
	void DebugGUI();
	/// <summary>統計情報と3種のエミッター設定(全シーン共通のインスペクタ用)</summary>
	void EmitterInspectorGUI();

	//========================================================================*/
	//* Getter
	IGPUEmitter& GetParticleCSEmitter(int index);
	SphereEmitter& GetSphereEmitter(int index);
	TextureBasedEmitter& GetParticleCSEmitterTexture(int index);
	MeshSurfaceEmitter& GetParticleCSEmitterSurface(int index);

private:

	void InitParticleCS();
	void InitInstance(ParticleCSInstance& CSInstance, size_t instanceSize);
	void InitGPUTimer();
	void InitSplat();
	void SplatDraw(); // コンピュート・スプラット描画(クリア→点描→加算合成)
	void AliveCountDataReadBack();
	void UpdatePerViewData(const Math::Matrix4x4& billboardMatrix);

	void UpdateGPUEmitter();

	//========================================================================*/
	//* Dispatch
	void UpdateParticleCSDispatch();
	void EmitterDispatch();
	void ClearWriteColorDispatch(); // ピンポン: 書き込み先プールのcolorを0クリア

private:

	DXCom* dxcommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	uint32_t particleCSInstanceCount_;

	// プール2枚化(完全ピンポン): Drawが読む trans/color のみ2枚持つ。
	// writeIdx_ を毎フレーム反転し、readIdx = writeIdx_^1。書き込みは[writeIdx_]、読み取りは[readIdx]。
	ParticleCSInstance transCSInstance_[2];
	ParticleCSInstance scaleCSInstance_;
	ParticleCSInstance timeCSInstance_;
	ParticleCSInstance velocityCSInstance_;
	ParticleCSInstance colorCSInstance_[2];
	ParticleCSInstance flagsCSInstance_;
	int writeIdx_ = 0; // 今フレームの書き込み先プールのインデックス(Dispatch冒頭で反転)

	Graphics::Material particleCSMaterial_;
	ComPtr<ID3D12Resource> perViewResource_[DXC::kFrameCount_];
	PerView* perViewData_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> perFrameResource_[DXC::kFrameCount_];
	PerFrame* perFrameDataGPU_[DXC::kFrameCount_];
	PerFrame perFrameData_;

	ComPtr<ID3D12Resource> freeListIndexResource_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListIndexUAVHandle_;
	ComPtr<ID3D12Resource> freeListTailIndexResource_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListTailIndexUAVHandle_;
	ComPtr<ID3D12Resource> freeListResource_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListUAVHandle_;

	ComPtr<ID3D12CommandSignature> drawIndexedSignature_;
	ComPtr<ID3D12Resource> aliveDrawArgs_[DXC::kFrameCount_];
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> ArgsUAVHandle_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> drawAliveIndex_[DXC::kFrameCount_];
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> drawAliveUAVHandle_[DXC::kFrameCount_];
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> drawAliveSRVHandle_[DXC::kFrameCount_];
	ComPtr<ID3D12Resource> aliveReadback_[DXC::kFrameCount_];

	// --- コンピュート・スプラット描画 ---
	ComPtr<ID3D12Resource> splatAccumResource_; // 蓄積バッファ(1px=4uint固定小数)
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> splatAccumUAVHandle_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> splatAccumSRVHandle_;
	ComPtr<ID3D12Resource> splatParamResource_;
	SplatParamCB* splatParamData_ = nullptr; // 永続マップ(enableDepthTestを毎フレーム更新)
	uint32_t splatAccumElementCount_ = 0;
	bool useComputeSplat_ = true;    // true:コンピュート・スプラット / false:従来ラスタ(1/4解像度)
	bool useFullResolution_ = true;  // 描画解像度: true=フルサイズ(深度テスト有効) / false=1/4扱い(深度テスト無効)。useComputeSplat_とは独立

	// Stage2(フェンス並走化): splat時に compute_N の graphics_{N-1} 待ちを省き Update_{N+1} ∥ Draw_N にする。
	bool useOverlap_ = true;             // true:オーバーラップ有効(A/B計測用にトグル可)
	bool prevUseComputeSplat_ = false;   // 前フレームのsplat有無。2フレーム連続splat時のみ並走(切替フレームのscaleハザード回避)

	std::vector<EmitterInfo> csEmitters_;

	std::vector<int> sphereEmitters_;
	std::vector<int> textureBasedEmitters_;
	std::vector<int> MeshSurfaceEmitters_;

	int csEmitterIndex_ = 0;
	int aliveCount_ = 0;

	int sphereEmitterIndex_ = 0;
	int textureBasedEmitterIndex_ = 0;
	int MeshSurfaceEmitterIndex_ = 0;

	// 5000万パーティクル (48 * 1024 * 1024 = 50,331,648)。kThreadsPerRow(1024*1024)の倍数にして2D Dispatchを綺麗に割り切らせる。
	// ※この値はシェーダ側の kMaxParticles (CSParticle.hlsli) と必ず一致させること
	uint32_t numParticles = 50331648;
	uint32_t threadsPerGroup = 1024;
	int threadGroupSize_ = 1024;

	DXC::GPUTimer gpuTimerGraphics;
	DXC::GPUTimer gpuTimerCompute;

#ifdef _DEBUGMODE
	std::unique_ptr<Editor::GPUParticleSystemEditor> editor_;
#endif // _DEBUGMODE
};
