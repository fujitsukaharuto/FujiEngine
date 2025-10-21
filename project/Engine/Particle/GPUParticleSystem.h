#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <map>
#include "Particle.h"
#include "Model.h"
#include "Object3d.h"
#include "Math/Matrix/MatrixCalculation.h"

using Microsoft::WRL::ComPtr;


struct PerView {
	Matrix4x4 viewProjection;
	Matrix4x4 billboardMatrix;
};

struct PerFrame {
	float time;
	float deltaTime;
};

struct EmitterSphere {
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

	// distanceEmit
	Vector3 prevTranslate;
	float padding4;

};

struct EmitterTexture {
	Vector3 translate;
	float radius;
	uint32_t count;
	float lifeTime;
	float frequency;
	float frequencyTime;
	uint32_t emit;

	// color
	Vector3 colorMax;
	//float padding;
	Vector3 colorMin;
	float padding2;

	// velocity
	Vector3 baseVelocity;
	float velocityRandMax;
	float velocityRandMin;

};

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

struct AcceleFiled {
	Vector3 Accele;
	AABB area;
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

	struct GPUParticleEmitter {
		EmitterSphere* emitter;
		ComPtr<ID3D12Resource> emitterResource;
		bool isEmit = false;
		int emitterIndex = 0;
	};

	struct GPUParticleEmitterTexture {
		EmitterTexture* emitter;
		ComPtr<ID3D12Resource> emitterResource;
		Texture* textureForEmit;
		bool isEmit = false;
		int emitterIndex = 0;
	};

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
	int InitGPUEmitterTexture();
	int InitGPUEmitterSurface(const std::string& fileName);

	void ParticleCSDebugGUI();
	void ParticleTexCSDebugGUI();
	void ParticleSurfaceCSDebugGUI();

	GPUParticleEmitter& GetParticleCSEmitter(int index);
	GPUParticleEmitterTexture& GetParticleCSEmitterTexture(int index);
	GPUParticleEmitterSurface& GetParticleCSEmitterSurface(int index);

private:

	void InitParticleCS();
	void UpdatePerViewData(const Matrix4x4& billboardMatrix);
	void DrawParticleCS(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView);

	void UpdateGPUEmitter();
	void UpdateGPUEmitterTexture();
	void UpdateGPUEmitterSurface();

	void UpdateParticleCSDispatch();
	void EmitterDispatch();
	void EmitterTextureDispatch();
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


	std::vector<GPUParticleEmitter> csEmitters_;
	std::vector<GPUParticleEmitterTexture> csEmitterTexs_;
	std::vector<GPUParticleEmitterSurface> csEmitterSurfces_;
	
	int csEmitterIndex_ = 0;
	int csEmitterTexIndex_ = 0;
	int csEmitterSurIndex_ = 0;

	uint32_t numParticles = 1048576;
	uint32_t threadsPerGroup = 1024;
	int threadGroupSize_ = 1024;

#ifdef _DEBUG
	int editCSEmitInd_;
	int editCSEmitTexInd_;
	int editCSEmitSurfaceInd_;
#endif // _DEBUG
};
