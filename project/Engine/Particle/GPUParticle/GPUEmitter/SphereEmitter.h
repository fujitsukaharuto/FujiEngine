#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "../IGPUParticleEmitter.h"
#include "Math/Matrix/MatrixCalculation.h"

using Microsoft::WRL::ComPtr;


/// <summary>
/// GPUパーティクルエミッター
/// </summary>
struct EmitterSphere {
	Vector3 translate;
	float radius;

	Vector3 scale;
	uint32_t count;

	Vector3 colorMax;
	float  lifeTime;

	Vector3 colorMin;
	float frequency;

	Vector3 baseVelocity;
	float velocityRandMax;

	float velocityRandMin;
	uint32_t emit;
	uint32_t isDistance;
	float frequencyTime;

	Vector3 prevTranslate;
	uint32_t emitShapeType;

	Quaternion rotation;

	uint32_t emitVeloType;
};

class SphereEmitter : public IGPUEmitter {
public:
	EmitterSphere* data_ = nullptr;
	ComPtr<ID3D12Resource> resource_;
	bool isEmit_ = false;

	SphereEmitter(DXCom* dx);

	void Update(float deltaTime) override;
	void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) override;
	void DebugGUI() override;

	bool IsEmit() const override { return isEmit_; }
	void SetEmit(bool state) override { isEmit_ = state; }
};