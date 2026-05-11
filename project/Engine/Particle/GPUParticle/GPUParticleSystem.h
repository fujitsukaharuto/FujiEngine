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

struct ParticleCSInstance {
	ComPtr<ID3D12Resource> particleCSInstancing_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSSRVHandle_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSUAVHandle_;
};

/// <summary>
/// GPUパーティクル管理クラス
/// </summary>
class GPUParticleSystem {
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

	void DebugGUI();
	void RenderPerformanceStats();
	void RenderEmitterList(std::vector<int>& emitterIndices, int& currentIdx, PipelinePhase phase);
	void ParticleCSDebugGUI();
	void ParticleTexCSDebugGUI();
	void ParticleSurfaceCSDebugGUI();

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
	void AliveCountDataReadBack();
	void UpdatePerViewData(const Math::Matrix4x4& billboardMatrix);

	void UpdateGPUEmitter();

	//========================================================================*/
	//* Debug
	void LoadPopUpGUI(int id, PipelinePhase type);
	void MouseTransGuizmo();

	//========================================================================*/
	//* Dispatch
	void UpdateParticleCSDispatch();
	void EmitterDispatch();

private:

	DXCom* dxcommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	uint32_t particleCSInstanceCount_;

	ParticleCSInstance transCSInstance_;
	ParticleCSInstance scaleCSInstance_;
	ParticleCSInstance timeCSInstance_;
	ParticleCSInstance velocityCSInstance_;
	ParticleCSInstance colorCSInstance_;
	ParticleCSInstance flagsCSInstance_;

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
	ComPtr<ID3D12Resource> drawAliveIndex_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> drawAliveUAVHandle_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> drawAliveSRVHandle_;
	ComPtr<ID3D12Resource> aliveReadback_[DXC::kFrameCount_];

	std::vector<EmitterInfo> csEmitters_;

	std::vector<int> sphereEmitters_;
	std::vector<int> textureBasedEmitters_;
	std::vector<int> MeshSurfaceEmitters_;

	int csEmitterIndex_ = 0;
	int aliveCount_ = 0;

	int sphereEmitterIndex_ = 0;
	int textureBasedEmitterIndex_ = 0;
	int MeshSurfaceEmitterIndex_ = 0;

	uint32_t numParticles = 60485760;
	uint32_t threadsPerGroup = 1024;
	int threadGroupSize_ = 1024;

	DXC::GPUTimer gpuTimerGraphics;
	DXC::GPUTimer gpuTimerCompute;

	bool isMouseTracking_ = true;
	Math::Trans mouseTrans_;

#ifdef _DEBUGMODE
	int editCSEmitInd_;
	int editCSEmitTexInd_;
	int editCSEmitSurfaceInd_;
#endif // _DEBUG
};
