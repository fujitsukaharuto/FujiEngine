#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <map>
#include "../Particle.h"
#include "GPUEmitter/SphereEmitter.h"
#include "GPUEmitter/TextureBasedEmitter.h"
#include "GPUEmitter/MeshSurefaceEmitter.h"

#include "Model.h"
#include "Object3d.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクル用行列データ
/// </summary>
struct PerView {
	Math::Matrix4x4 viewProjection;
	Math::Matrix4x4 billboardMatrix;
};

/// <summary>
/// GPUパーティクル用フレームデータ
/// </summary>
struct PerFrame {
	float time;
	float deltaTime;
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

/// <summary>
/// GPUパーティクル管理クラス
/// </summary>
class GPUParticleSystem {
public:
	GPUParticleSystem();
	~GPUParticleSystem();

public:

	void Initialize(DXCom* pDxcom, SRVManager* srvManager);
	void Finalize();
	void Update(const Math::Matrix4x4& billboardMatrix);
	void Draw(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView);
	void ResetEmitters();
	void InitDefoultEmitter();

	int InitGPUEmitter(int returnMod = 0);
	int InitGPUEmitterTexture(const std::string& fileName = "white2x2.png");
	int InitGPUEmitterSurface(const std::string& fileName);

	void ParticleCSDebugGUI();
	void ParticleTexCSDebugGUI();
	void ParticleSurfaceCSDebugGUI();

	//========================================================================*/
	//* Getter
	IGPUEmitter& GetParticleCSEmitter(int index);
	SphereEmitter& GetSphereEmitter(int index);
	TextureBasedEmitter& GetParticleCSEmitterTexture(int index);
	MeshSurefaceEmitter& GetParticleCSEmitterSurface(int index);

private:

	void InitParticleCS();
	void UpdatePerViewData(const Math::Matrix4x4& billboardMatrix);
	void DrawParticleCS(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView);

	void UpdateGPUEmitter();

	//========================================================================*/
	//* Dispatch
	void UpdateParticleCSDispatch();
	void EmitterDispatch();

private:

	DXCom* dxcommon_;
	SRVManager* srvManager_;
	Camera* camera_;


	ComPtr<ID3D12Resource> particleCSInstancing_;
	uint32_t particleCSInsstanceCount_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSSRVHandle_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSUAVHandle_;
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


	std::vector<EmitterInfo> csEmitters_;

	std::vector<int> sphereEmitters_;
	std::vector<int> textureBasedEmitters_;
	std::vector<int> MeshSurefaceEmitters_;

	int csEmitterIndex_ = 0;

	int sphereEmitterIndex_ = 0;
	int textureBasedEmitterIndex_ = 0;
	int MeshSurefaceEmitterIndex_ = 0;

	uint32_t numParticles = 10485760;
	uint32_t threadsPerGroup = 1024;
	int threadGroupSize_ = 1024;

#ifdef _DEBUG
	int editCSEmitInd_;
	int editCSEmitTexInd_;
	int editCSEmitSurfaceInd_;
#endif // _DEBUG
};
