#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <map>
#include "../Particle.h"
#include "GPUEmitter/SphereEmitter.h"
#include "GPUEmitter/TextureBasedEmitter.h"

#include "Model.h"
#include "Object3d.h"
#include "Math/Matrix/MatrixCalculation.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクル用行列データ
/// </summary>
struct PerView {
	Matrix4x4 viewProjection;
	Matrix4x4 billboardMatrix;
};

/// <summary>
/// GPUパーティクル用フレームデータ
/// </summary>
struct PerFrame {
	float time;
	float deltaTime;
};

/// <summary>
/// GPUパーティクル表面エミッター
/// </summary>
struct EmitterSurface {
	Vector3 translate;
	float padding;
	Vector3 scale;
	float radius;
	uint32_t count;
	float lifeTime;
	float frequency;
	float frequencyTime;
	uint32_t emit;

	// color
	Vector3 colorMax;
	Vector3 colorMin;
	float padding2;

	// velocity
	Vector3 baseVelocity;
	float velocityRandMax;
	float velocityRandMin;

	int triangleCount;
};

/// <summary>
/// 加速場
/// </summary>
struct AcceleFiled {
	Vector3 Accele;
	AABB area;
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

	/// <summary>
	/// 表面エミッターデータ
	/// </summary>
	struct GPUParticleEmitterSurface {
		EmitterSurface* emitter;
		ComPtr<ID3D12Resource> emitterResource;
		ComPtr<ID3D12Resource> verticesResource;
		ComPtr<ID3D12Resource> indiciesResource;
		ComPtr<ID3D12Resource> areasResource;
		uint32_t verticesIndex;
		uint32_t indiciesIndex;
		uint32_t areasIndex;

		bool isEmit = false;
		int emitterIndex = 0;
	};


	void Initialize(DXCom* pDxcom, SRVManager* srvManager);
	void Finalize();
	void Update(const Matrix4x4& billboardMatrix);
	void Draw(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView);

	int InitGPUEmitter();
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
	GPUParticleEmitterSurface& GetParticleCSEmitterSurface(int index);

private:

	void InitParticleCS();
	void UpdatePerViewData(const Matrix4x4& billboardMatrix);
	void DrawParticleCS(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView);

	void UpdateGPUEmitter();
	void UpdateGPUEmitterSurface();

	//========================================================================*/
	//* Dispatch
	void UpdateParticleCSDispatch();
	void EmitterDispatch();
	void EmitterSurfaceDispatch();

private:

	DXCom* dxcommon_;
	SRVManager* srvManager_;
	Camera* camera_;


	ComPtr<ID3D12Resource> particleCSInstancing_;
	uint32_t particleCSInsstanceCount_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSSRVHandle_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> particleCSUAVHandle_;
	Material particleCSMaterial_;
	ComPtr<ID3D12Resource> perViewResource_;
	PerView* perViewData_;
	ComPtr<ID3D12Resource> perFrameResource_;
	PerFrame* perFrameData_;

	ComPtr<ID3D12Resource> freeListIndexResource_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListIndexUAVHandle_;
	ComPtr<ID3D12Resource> freeListResource_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> freeListUAVHandle_;


	std::vector<EmitterInfo> csEmitters_;
	std::vector<GPUParticleEmitterSurface> csEmitterSurfces_;

	std::vector<int> sphereEmitters_;
	std::vector<int> textureBasedEmitters_;
	std::vector<int> MeshSurefaceEmitters_;

	int csEmitterIndex_ = 0;

	int sphereEmitterIndex_ = 0;
	int textureBasedEmitterIndex_ = 0;
	int MeshSurefaceEmitterIndex_ = 0;

	uint32_t numParticles = 1048576;
	uint32_t threadsPerGroup = 1024;
	int threadGroupSize_ = 1024;

#ifdef _DEBUG
	int editCSEmitInd_;
	int editCSEmitTexInd_;
	int editCSEmitSurfaceInd_;
#endif // _DEBUG
};
